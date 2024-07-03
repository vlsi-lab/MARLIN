import torch
from torch import nn
from torch.nn import functional as F
from torch.autograd import Function


class Conv2d(nn.Conv2d):
    def __init__(self, in_channels, out_channels, kernel_size, stride=1, padding=0, groups=1, dilation=1, bias=False, act_bit=8, weight_bit=8, bias_bit=8):
        super(Conv2d, self).__init__(in_channels, out_channels, kernel_size, stride, padding, groups, dilation, bias)
        self.act_bit = act_bit  # activation precision
        self.weight_bit = weight_bit  # weight precision
        self.bias_bit = bias_bit  # bias precision
        self.min_v_w = - 2 ** (self.weight_bit - 1)
        self.max_v_w = 2 ** (self.weight_bit - 1) - 1
        self.min_v_b = - 2 ** (self.bias_bit - 1)
        self.max_v_b = 2 ** (self.bias_bit - 1) - 1
        self.min_v_a = - 2 ** (self.act_bit - 1)
        self.max_v_a = 2 ** (self.act_bit - 1) - 1
        torch.nn.init.kaiming_normal_(self.weight)  # init weight
        self.quantize = quantize_tensor.apply
        self.data_shape = None

    def forward(self, x):
        """
        Compute the quantized convolution with or without quantized bias. Activations are either quantized with scale
        quantization from float, or re-quantized after being quantized with pact. The latter double quantization does
        not influence the dynamic or tensor values as the scaling factor with PACT (that is the clipping factor) is also
        the scaling factor used in the scale quantization.
        @param x: quantized or float input tensor
        @return: output tensor
        """
        self.data_shape = x.shape  # tensor size saved and used to evaluate the overall number of MAC operations
        xq = self.quantize(x, self.min_v_a, self.max_v_a)  # re-quantize activations, does not change activation value of layers after a PACT ReLU
        wq = self.quantize(self.weight, self.min_v_w, self.max_v_w)  # quantize weights
        if self.bias is None:
            bq = None
        else:
            bq = self.quantize(self.bias, self.min_v_b, self.max_v_b)  # quantize bias
        y = F.conv2d(xq, wq, bq, self.stride, self.padding, self.dilation, self.groups)
        return y


class Linear(nn.Linear):
    def __init__(self, in_channels, out_channels, bias=True, act_bit=8, weight_bit=8, bias_bit=8):
        super(Linear, self).__init__(in_channels, out_channels, bias)
        self.act_bit = act_bit  # activation precision
        self.weight_bit = weight_bit  # weight precision
        self.bias_bit = bias_bit  # bias precision
        self.min_v_w = - 2 ** (self.weight_bit - 1)
        self.max_v_w = 2 ** (self.weight_bit - 1) - 1
        self.min_v_b = - 2 ** (self.bias_bit - 1)
        self.max_v_b = 2 ** (self.bias_bit - 1) - 1
        self.min_v_a = - 2 ** (self.act_bit - 1)
        self.max_v_a = 2 ** (self.act_bit - 1) - 1
        self.quantize = quantize_tensor.apply
        torch.nn.init.kaiming_normal_(self.weight)  # init weight
        self.data_shape = None

    def forward(self, x):
        """
        Same as Conv2d described above
        """
        self.data_shape = x.shape  # tensor size saved and used to evaluate the overall number of MAC operations
        xq = self.quantize(x, self.min_v_a, self.max_v_a)  # re-quantize activations, does not change activation value of layers after a PACT ReLU
        wq = self.quantize(self.weight, self.min_v_w, self.max_v_w)  # quantize weights
        if self.bias is None:
            bq = None
        else:
            bq = self.quantize(self.bias, self.min_v_b, self.max_v_b)  # quantize bias

        y = F.linear(xq, wq, bq)
        return y


class BatchNorm2d(nn.Module):  # quantized batch norm
    def __init__(self, num_features, momentum=0.9, eps=1e-5, freeze_norm=False, batch_norm_bit=16):
        super(BatchNorm2d, self).__init__()
        self.batch_norm_bit = batch_norm_bit  # all the batchnorm's parameters are quantized with the same precision
        self.min_v = - 2 ** (self.batch_norm_bit - 1)
        self.max_v = 2 ** (self.batch_norm_bit - 1) - 1

        # Batch norm parameters
        self.eps = eps
        self.bn_beta = nn.Parameter(torch.zeros(num_features))
        self.bn_gamma = nn.Parameter(torch.ones(num_features))
        self.momentum = momentum
        self.running_mean = nn.Parameter(torch.zeros(num_features))  # running estimates computed only during
        self.running_var = nn.Parameter(torch.ones(num_features))    # the training, when model.train() is called
        self.freeze_norm = freeze_norm  # stop running mean/var during training, can be useful in case of fused or quantized retraining to avoid instability
        self.batch_norm = BatchNorm2DFunction.apply

    def forward(self, x):
        out, r_mean, r_var = self.batch_norm(x, self.bn_gamma, self.bn_beta, self.eps, self.momentum,
                                             self.running_mean, self.running_var, self.min_v, self.max_v,
                                             self.training, self.freeze_norm)
        if self.training and not self.freeze_norm:
            with torch.no_grad():
                self.running_mean.copy_(r_mean)
                self.running_var.copy_(r_var)
        return out


class BatchNorm2DFunction(torch.autograd.Function):
    """
    Explicit forward and backward batchnorm implementation for quantization and further experiments.
    This class is an implementation of the formula described in this paper https://proceedings.mlr.press/v37/ioffe15.pdf
    """
    @staticmethod
    def forward(ctx, in_tensor, gamma, beta, eps, momentum, running_mean, running_var, min_v, max_v, training, freeze_norm):
        assert in_tensor.ndim == 4  # N, C, H, W
        ctx.save_for_backward(in_tensor, gamma, beta)
        ctx.eps = eps

        if training:
            sum_v = in_tensor.sum(dim=(0, 2, 3))
            var = in_tensor.var(unbiased=True, dim=(0, 2, 3))
            N = in_tensor.numel() / in_tensor.size(1)
            ctx.N = N
            mean = sum_v / N
            sqrt_var = torch.sqrt(var + eps)
            ctx.sum_v = sum_v
            ctx.sqrt_var = sqrt_var

            if not freeze_norm:
                running_mean = momentum * mean + (1 - momentum) * running_mean
                running_var = momentum * var * N / (N - 1) + (1 - momentum) * running_var

        else:
            mean = running_mean
            sqrt_var = torch.sqrt(running_var + eps)
        gamma_sqrt = gamma.div(sqrt_var)

        gamma_sqrt = quantize_tensor_function(gamma_sqrt, min_v, max_v)
        beta = quantize_tensor_function(beta, min_v, max_v)
        mean = quantize_tensor_function(mean, min_v, max_v)
        out = (in_tensor - unsqueeze_all(mean)) * unsqueeze_all(gamma_sqrt) + unsqueeze_all(beta)

        return out, running_mean, running_var

    @staticmethod
    def backward(ctx, grad_out, _, __,):
        X, gamma, beta, = ctx.saved_tensors
        grad_in, dgamma, dbeta = batch_norm_backward(grad_out, X, ctx.sum_v, ctx.sqrt_var, ctx.N, gamma, ctx.eps)

        return grad_in, dgamma, dbeta, None, None, None, None, None, None, None


def batch_norm_backward(grad_out, X, sum_v, sqrt_var, N, gamma, eps):
    """
    Backpropagation formula taken from
    https://kratzert.github.io/2016/02/12/understanding-the-gradient-flow-through-the-batch-normalization-layer.html
    Baseline taken from
    https://pytorch.org/tutorials/intermediate/custom_function_conv_bn_tutorial.html#a-comparison-of-memory-usage
    Modified to include bias training
    """
    dbeta = grad_out.sum(dim=(0, 2, 3))  # gradient of beta
    x_meno_sum = (X - unsqueeze_all(sum_v) / N)
    unsqueezed_gamma = unsqueeze_all(gamma)
    unsqueezed_sqrtvareps = unsqueeze_all(sqrt_var + eps)
    dgamma = (x_meno_sum / unsqueeze_all(sqrt_var) * grad_out).sum(dim=(0, 2, 3))  # gradient of gamma
    tmp = - (x_meno_sum * grad_out * unsqueezed_gamma).sum(dim=(0, 2, 3))
    d_denom = tmp / (sqrt_var + eps) ** 2
    d_var = d_denom / (2 * sqrt_var)
    d_mean_dx_tmp = grad_out * unsqueezed_gamma / unsqueezed_sqrtvareps
    grad_input_tmp = (X * unsqueeze_all(d_var * N) + unsqueeze_all(-d_var * sum_v)) * 2 / ((N - 1) * N)
    grad_input_tmp1 = (grad_input_tmp + unsqueeze_all(-d_mean_dx_tmp.sum(dim=(0, 2, 3)) / N)) * unsqueezed_sqrtvareps
    grad_input = (grad_input_tmp1 + grad_out * unsqueezed_gamma) / unsqueezed_sqrtvareps
    return grad_input, dgamma, dbeta


def unsqueeze_all(t):
    return t[None, :, None, None]


class quantize_tensor(Function):
    @staticmethod
    def forward(ctx, real_tensor, min_v, max_v):
        ctx.save_for_backward(real_tensor)
        return quantize_tensor_function(real_tensor, min_v, max_v)

    @staticmethod
    def backward(ctx, grad_out_tensor):  # straight-through estimator
        return grad_out_tensor, None, None


def quantize_tensor_function(real_tensor, min_v, max_v):
        t_max = torch.max(torch.abs(torch.min(real_tensor)), torch.abs(torch.max(real_tensor))).item()
        if t_max == 0.0:
            t_max = 1.0
        scaling_factor = max_v / t_max  # best usage of quantized range
        return torch.clamp(torch.round(scaling_factor * real_tensor), min=min_v, max=max_v) / scaling_factor


class ReLu(nn.Module):
    def __init__(self, act_bit, alpha=10.0):
        """
        ReLU implemented with PACT
        @param act_bit: activations precision
        @param alpha: initial learnable scaling factor
        """
        super(ReLu, self).__init__()
        self.alpha = nn.Parameter(torch.tensor(alpha))
        self.relu = ParametrizedActivationClipping.apply
        self.scale = 2 ** (act_bit - 1) - 1
        self.max_v = 2 ** (act_bit - 1) - 1

    def forward(self, x):
        out = self.relu(x, self.alpha, self.max_v)
        return out

class ParametrizedActivationClipping(Function):
    """
    Perform parametrized activation clipping as described in PACT https://arxiv.org/pdf/1805.06085.pdf
    """
    @staticmethod
    def forward(ctx, x, alpha, max_v):
        ctx.save_for_backward(x, alpha)
        y = torch.clamp(x, min=0, max=alpha.item())
        scale_alpha = max_v / alpha
        y_q = torch.clamp(torch.round(y * scale_alpha), min=-max_v, max=max_v) / scale_alpha
        return y_q

    @staticmethod
    def backward(ctx, dLdy_q):
        x, alpha, = ctx.saved_tensors
        lower_bound = x < 0
        upper_bound = x > alpha
        x_range = ~(lower_bound | upper_bound)
        grad_alpha = torch.sum(dLdy_q * torch.ge(x, alpha).float()).view(-1)
        return dLdy_q * x_range.float(), grad_alpha, None


class FusedConvBN2DFunction(torch.autograd.Function):
    """
    Custom fused conv2d/batch norm, baseline with hard-coded stride/padding/momentum etc. taken from pytorch website,
    https://pytorch.org/tutorials/intermediate/custom_function_conv_bn_tutorial.html#a-comparison-of-memory-usage
    Backprop intuition for conv is also taken from:
    https://medium.com/apache-mxnet/transposed-convolutions-explained-with-ms-excel-52d13030c7e8
    Batch norm implementation is taken from the paper https://proceedings.mlr.press/v37/ioffe15.pdf
    Additional links for batch norms are
    Pytorch base definition https://pytorch.org/docs/stable/generated/torch.nn.BatchNorm2d.html
    Gamma and beta initialization https://github.com/pytorch/pytorch/blob/master/torch/nn/modules/batchnorm.py#L67-L77
    Running mean/var and GPU issues:
    https://discuss.pytorch.org/t/updating-running-mean-and-running-var-in-a-custom-batchnorm/77464
    Additional solution to GPU issues:
    https://discuss.pytorch.org/t/implementing-batchnorm-in-pytorch-problem-with-updating-self-running-mean-and-self-running-var/49314/6
    """
    @staticmethod
    def forward(ctx, in_tensor, conv_weight, gamma, beta, padding, stride, eps, exp_avg_f, running_mean, running_var,
                w_fold, b_fold, max_v_a, max_v_w, max_v_b, training, freeze_norm):
        assert in_tensor.ndim == 4  # N, C, H, W
        ctx.save_for_backward(in_tensor, conv_weight, gamma, beta)
        ctx.padding = padding
        ctx.stride = stride
        ctx.dilation = 1
        ctx.groups = 1
        ctx.eps = eps

        if training:
            X_conv_out = F.conv2d(in_tensor, conv_weight, None, stride, padding, 1, 1)
            ctx.X_conv_out = X_conv_out
            sum_v = X_conv_out.sum(dim=(0, 2, 3))
            var = X_conv_out.var(unbiased=True, dim=(0, 2, 3))
            N = X_conv_out.numel() / X_conv_out.size(1)
            ctx.N = N
            mean = sum_v / N
            sqrt_var = torch.sqrt(var + eps)  # square root of the variance
            ctx.sum_v = sum_v
            ctx.sqrt_var = sqrt_var
            if not freeze_norm:
                running_mean = exp_avg_f * mean + (1 - exp_avg_f) * running_mean
                running_var = exp_avg_f * var * N / (N - 1) + (1 - exp_avg_f) * running_var

        else:
            N = in_tensor.numel() / in_tensor.size(1)
            ctx.N = N
            mean = running_mean
            ctx.sum = mean * N
            sqrt_var = torch.sqrt(running_var + eps)
            ctx.sqrt_var = sqrt_var

        w_bn = torch.diag(gamma.div(sqrt_var))
        w_fold = torch.mm(w_bn, conv_weight.view(conv_weight.shape[0], -1)).view(conv_weight.size())
        b_fold = beta - gamma.mul(mean).div(sqrt_var)
        b_fold = quantize_tensor_function(b_fold, -max_v_b, max_v_b)
        w_fold = quantize_tensor_function(w_fold, -max_v_w, max_v_w)
        in_tensor = quantize_tensor_function(in_tensor, -max_v_a, max_v_a)
        out = F.conv2d(in_tensor, w_fold, b_fold, stride, padding, 1, 1)

        return out, running_mean, running_var, w_fold, b_fold

    @staticmethod
    def backward(ctx, grad_out, _, __, ___, ____):
        X, conv_weight, gamma, beta, = ctx.saved_tensors
        X_conv_out = ctx.X_conv_out
        grad_out, dgamma, dbeta = batch_norm_backward(grad_out, X_conv_out, ctx.sum_v, ctx.sqrt_var, ctx.N, gamma, ctx.eps)
        grad_X, grad_weight = convolution_backward(grad_out, X, conv_weight, ctx.padding, ctx.stride, ctx.dilation, ctx.groups)
        return grad_X, grad_weight, dgamma, dbeta, None, None, None, None, None, None, None, None, None, None, None, None, None


def convolution_backward(grad_out, X, weight, padding, stride, dilation, groups):
    grad_input = torch.nn.grad.conv2d_input(X.shape, weight, grad_out, stride, padding, dilation, groups)
    grad_weight = torch.nn.grad.conv2d_weight(X, weight.shape, grad_out, stride, padding, dilation, groups)
    return grad_input, grad_weight


class FusedConvBN(nn.Module):
    """
    Class that wraps the FusedConvBN2DFunction parameters initialization
    """
    def __init__(self, in_channels, out_channels, kernel_size, padding=(1, 1), stride=(1, 1),
                 eps=1e-5, bn_momentum=0.1, freeze_norm=False,
                 act_bit=16, weight_bit=16, bias_bit=16):
        super(FusedConvBN, self).__init__()
        self.act_bit = act_bit
        self.weight_bit = weight_bit
        self.bias_bit = bias_bit
        self.max_v_w = 2 ** (self.weight_bit - 1) - 1
        self.max_v_b = 2 ** (self.bias_bit - 1) - 1
        self.max_v_a = 2 ** (self.act_bit - 1) - 1

        # Conv parameters
        self.conv = FusedConvBN2DFunction.apply
        weight_shape = (out_channels, in_channels, kernel_size[0], kernel_size[1])
        self.conv_weight = nn.Parameter(torch.empty(*weight_shape))
        torch.nn.init.kaiming_normal_(self.conv_weight)
        self.padding = padding
        self.stride = stride

        # Batch norm parameters
        self.eps = eps
        self.bn_beta = nn.Parameter(torch.zeros(out_channels))
        self.bn_gamma = nn.Parameter(torch.ones(out_channels))
        self.exp_avg_f = bn_momentum
        self.running_mean = nn.Parameter(torch.zeros(out_channels))
        self.running_var = nn.Parameter(torch.ones(out_channels))
        self.freeze_norm = freeze_norm

        self.weight = nn.Parameter(torch.empty(*weight_shape))
        self.bias = nn.Parameter(torch.empty(out_channels))

    def forward(self, X):

        out, r_mean, r_var, w, b = self.conv(X, self.conv_weight, self.bn_gamma, self.bn_beta,
                                             self.padding, self.stride,
                                             self.eps, self.exp_avg_f, self.running_mean, self.running_var,
                                             self.weight, self.bias,
                                             self.max_v_a, self.max_v_w, self.max_v_b,
                                             self.training, self.freeze_norm)
        if self.training:
            with torch.no_grad():
                self.weight.copy_(w)
                self.bias.copy_(b)
                if not self.freeze_norm:
                    self.running_mean.copy_(r_mean)
                    self.running_var.copy_(r_var)

        return out

