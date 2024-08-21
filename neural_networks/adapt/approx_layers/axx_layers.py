"""
This file is part of the AdaPT framework, available at https://github.com/dimdano/adapt
This file was downloaded and modified in order to support runtime configuration of the approximate kernel, bias, and
quantization without requiring the packages "pytorch-quantization" and "nvidia-pyindex".
"""
import torch.utils.data
from .torch_utils import _ConvNd, _size_2_t, Union, Optional, _pair
import torch.nn as nn
from torch import Tensor
import math

from torch.utils.cpp_extension import load


class AdaPT_Linear_Function(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input, weight, bias, bias_, axx_linear_kernel, max_val):
        ctx.save_for_backward(input, weight, bias)
        ctx.bias_ = bias_
        t_max = torch.max(torch.abs(torch.min(weight)), torch.abs(torch.max(weight))).item()
        if t_max == 0.0:
            t_max = 1.0
        scaling_factor_w = max_val / t_max  
        quant_weight = torch.clamp(torch.round(scaling_factor_w * weight), min=-max_val, max=max_val).to(dtype=torch.int8)

        t_max = torch.max(torch.abs(torch.min(input)), torch.abs(torch.max(input))).item()
        if t_max == 0.0:
            t_max = 1.0
        scaling_factor_a = max_val / t_max  
        quant_input = torch.clamp(torch.round(scaling_factor_a * input), min=-max_val, max=max_val).to(dtype=torch.int8)

        if bias_:
            t_max = torch.max(torch.abs(torch.min(bias)), torch.abs(torch.max(bias))).item()
            if t_max == 0.0:
                t_max = 1.0
            scaling_factor_b = max_val / t_max  
            quant_bias = torch.clamp(torch.round(scaling_factor_b * bias), min=-max_val, max=max_val).to(dtype=torch.int8)
        else:
            quant_bias = None

        output = axx_linear_kernel.forward(quant_input, quant_weight)
        output = output / (scaling_factor_a * scaling_factor_w)

        if bias_:
            return output + quant_bias / scaling_factor_b
        return output

    @staticmethod
    def backward(ctx, grad_output):
        input, weight, bias = ctx.saved_tensors
        grad_input = grad_weight = grad_bias = None
        bias_ = ctx.bias_

        if ctx.needs_input_grad[0]:
            grad_input = grad_output.mm(weight)
        if ctx.needs_input_grad[1]:
            grad_weight = grad_output.t().mm(input)
        if bias_ and ctx.needs_input_grad[2]:
            grad_bias = grad_output.sum(0)

        return grad_input, grad_weight, grad_bias, None, None, None


class AdaPT_Linear(nn.Module):
    def __init__(self, size_in, size_out, bias=True, axx_mult='bw_mult_9_9_0', freeze_weight=False, freeze_bias=False, nemo=False):

        super(AdaPT_Linear, self).__init__()

        self.size_in, self.size_out, self.bias_ = size_in, size_out, bias
        self.fn = AdaPT_Linear_Function.apply
        weight = torch.Tensor(size_out, size_in)
        self.weight = nn.Parameter(weight)  # nn.Parameter is a Tensor that's a module parameter.
        bias = torch.Tensor(size_out)
        self.bias = nn.Parameter(bias)
        self.axx_mult = axx_mult
        # Jit compilation method for cpp extention
        # set PyInit_ prefix to comply with the python module name
        self.nemo = nemo
        if self.nemo:
            self.max_val = 255
            self.axx_linear_kernel = load(name='PyInit_linear_' + axx_mult, sources=["./neural_networks/adapt/cpu-kernels-9x9/axx_linear.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        else:
            self.max_val = 127
            self.axx_linear_kernel = load(name='PyInit_linear_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-8x8/axx_linear.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        self.freeze_weight = freeze_weight
        self.weight.requires_grad = not self.freeze_weight
        self.freeze_bias = freeze_bias
        self.bias.requires_grad = not self.freeze_bias
        nn.init.kaiming_uniform_(self.weight, a=math.sqrt(5))  # weight init
        fan_in, _ = nn.init._calculate_fan_in_and_fan_out(self.weight)
        bound = 1 / math.sqrt(fan_in)
        nn.init.uniform_(self.bias, -bound, bound)  # bias init

        self.data_shape = None

    def forward(self, x):

        x = self.fn(x, self.weight, self.bias, self.bias_, self.axx_linear_kernel, self.max_val)
        self.data_shape = x.shape

        return x

    def update_kernel(self):
        if self.nemo:
            self.axx_linear_kernel = load(name='PyInit_linear_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-9x9/axx_linear.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        else:
            self.axx_linear_kernel = load(name='PyInit_linear_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-8x8/axx_linear.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        if self.freeze_weight:
            self.weight.requires_grad = False
        if self.freeze_bias:
            self.bias.requires_grad = False


class AdaPT_Conv2d_Function(torch.autograd.Function):

    @staticmethod
    def forward(ctx, input, weight, kernel_size, out_channels, bias_, axx_conv2d_kernel, bias=None, stride=1, padding=0,
                dilation=1, groups=1, max_val=127):
        ctx.save_for_backward(input, weight, bias)
        ctx.stride = stride
        ctx.padding = padding
        ctx.dilation = dilation
        ctx.groups = groups

        t_max = torch.max(torch.abs(torch.min(weight)), torch.abs(torch.max(weight))).item()
        if t_max == 0.0:
            t_max = 1.0
        scaling_factor_w = max_val / t_max  
        quant_weight = torch.clamp(torch.round(scaling_factor_w * weight), min=-max_val, max=max_val).to(dtype=torch.int8)

        t_max = torch.max(torch.abs(torch.min(input)), torch.abs(torch.max(input))).item()
        if t_max == 0.0:
            t_max = 1.0
        scaling_factor_a = max_val / t_max  
        quant_input = torch.clamp(torch.round(scaling_factor_a * input), min=-max_val, max=max_val).to(dtype=torch.int8)

        if bias_:
            t_max = torch.max(torch.abs(torch.min(bias)), torch.abs(torch.max(bias))).item()
            if t_max == 0.0:
                t_max = 1.0
            scaling_factor_b = max_val / t_max  
            quant_bias = torch.clamp(torch.round(scaling_factor_b * bias), min=-max_val, max=max_val).to(dtype=torch.int8)
        else:
            quant_bias = None

        # slow temporary version of grouped conv2d using simple conv2d (split+concat)
        # support only for group = input_dim = outputdim (i.e. mobilenetv2)
        if groups > 1:
            out = torch.empty(0)
            for i in range(0, groups):
                filters = quant_weight[i:(i + 1)]
                o = axx_conv2d_kernel.forward(quant_input[:, i:(i + 1)], filters, kernel_size, stride, padding)
                out = torch.cat((out, o), dim=1)

            out = out / (scaling_factor_a * scaling_factor_w)
            if bias_:
                return out + (quant_bias.reshape(1, out_channels, 1, 1) / scaling_factor_b)
            else:
                return out

        out = axx_conv2d_kernel.forward(quant_input, quant_weight, kernel_size, stride, padding)
        out = out / (scaling_factor_a * scaling_factor_w)
        if bias_:
            return out + (quant_bias.reshape(1, out_channels, 1, 1) / scaling_factor_b)

        return out

    @staticmethod
    def backward(ctx, grad_output):

        input, weight, bias = ctx.saved_variables
        stride = ctx.stride
        padding = ctx.padding
        dilation = ctx.dilation
        groups = ctx.groups
        grad_input = grad_weight = grad_bias = None

        if ctx.needs_input_grad[0]:
            grad_input = torch.nn.grad.conv2d_input(input.shape, weight, grad_output, stride, padding, dilation, groups)
        if ctx.needs_input_grad[1]:
            grad_weight = torch.nn.grad.conv2d_weight(input, weight.shape, grad_output, stride, padding, dilation, groups)
        if bias is not None and ctx.needs_input_grad[2]:
            grad_bias = grad_output.sum(dim=(0, 2, 3))

        return grad_input, grad_weight, grad_bias, None, None, None, None, None, None, None, None, None

class AdaPT_Conv2d_Function_nemo(torch.autograd.Function):

    @staticmethod
    def forward(ctx, input, weight, kernel_size, out_channels, bias_, axx_conv2d_kernel, bias=None, stride=1, padding=0, dilation=1, groups=1, padding_mode = 'zeros', act_bit = 9, weight_bit=9, bias_bit=8):
        #ctx.save_for_backward(input, weight, bias)  
        # in NEMO input, weights and biases are already integers,rescale them back to quantized fp to use forward pass and backward propagation 
        ctx.save_for_backward(input, weight, bias)  
        ctx.stride = stride
        ctx.padding = padding
        ctx.dilation = dilation
        ctx.groups = groups                 
        quant_weight = weight
        quant_input = input    
        if bias_:
            quant_bias = bias
        else:
            quant_bias = None
        #in case of normal padding_mode run approx_conv2d
        #TODO implement quant case for bias=True. Currently not needed for typical ConvNets
        #quant descriptors inside class is slower than using quant_nn.Conv2d instead
                        
        #if (amax == None):           
        #    return F.conv2d(input, weight, bias, stride, padding, dilation, groups)
        #else:
            
        #convert weights and biases to int8 (must change if approx_mult bits are higher)
        #They are already quantized with NEMO and integer, can be on unsigned8 bits as well (inputs)
        quant_input = quant_input.to(dtype=torch.int16)
        input_mask = torch.full(quant_input.size(), pow(2,act_bit)-1)
        input_mask = input_mask.to(dtype=torch.int16)
        quant_input = quant_input & input_mask
        quant_weight = quant_weight.to(dtype=torch.int16)
        weight_mask = torch.full(quant_weight.size(), pow(2,weight_bit)-1)
        weight_mask = weight_mask.to(dtype=torch.int16)
        quant_weight = quant_weight & weight_mask
        #slow temporary version of grouped conv2d using simple conv2d (split+concat)
        #support only for group = input_dim = outputdim (i.e. mobilenetv2)
        if groups > 1 : 
            out=torch.empty(0)
            for i in range(0,groups):
                filters = quant_weight[i:(i+1)]                   
                o =  axx_conv2d_kernel.forward(quant_input[:, i:(i+1)], filters, kernel_size, stride, padding) 
                out = torch.cat((out, o), dim=1)
                
            if bias_:
                return out + quant_bias.reshape(1,out_channels,1,1)   
            else: 
                return out
        
        if bias_:               
            #print("bias_")
            out = axx_conv2d_kernel.forward(quant_input, quant_weight, kernel_size, stride, padding)                
            return out + quant_bias.reshape(1,out_channels,1,1)
        out = axx_conv2d_kernel.forward(quant_input, quant_weight, kernel_size, stride, padding)
      
        return out

    @staticmethod
    def backward(ctx, grad_output):

        input, weight, bias = ctx.saved_variables
        stride = ctx.stride
        padding = ctx.padding
        dilation = ctx.dilation
        groups = ctx.groups
        grad_input = grad_weight = grad_bias = None            

        if ctx.needs_input_grad[0]:
            grad_input = torch.nn.grad.conv2d_input(input.shape, weight, grad_output, stride, padding, dilation, groups)
        if ctx.needs_input_grad[1]:
            grad_weight = torch.nn.grad.conv2d_weight(input, weight.shape, grad_output, stride, padding, dilation, groups)
        if bias is not None and ctx.needs_input_grad[2]:
            grad_bias = grad_output.sum(dim=(0, 2, 3))

        return grad_input, grad_weight, grad_bias, None, None, None, None, None, None, None, None, None, None, None, None, None, None




class AdaPT_Conv2d(_ConvNd):
    def __init__(
            self,
            in_channels: int,
            out_channels: int,
            kernel_size: _size_2_t,
            stride: _size_2_t = 1,
            padding: Union[str, _size_2_t] = 0,
            dilation: _size_2_t = 1,
            groups: int = 1,
            bias: bool = True,
            padding_mode: str = 'zeros',  # TODO: refine this type
            axx_mult='bw_mult_9_9_0',
            device=None,
            dtype=None,
            freeze_weight=False,
            nemo=False
    ) -> None:

        factory_kwargs = {'device': device, 'dtype': dtype}
        kernel_size_ = _pair(kernel_size)
        stride_ = _pair(stride)
        self.bias_ = bias  # added for case of none bias in order to run conv2d-cpu kernel
        padding_ = padding if isinstance(padding, str) else _pair(padding)
        dilation_ = _pair(dilation)
        self.axx_mult = axx_mult

        super(AdaPT_Conv2d, self).__init__(
            in_channels, out_channels, kernel_size_, stride_, padding_, dilation_,
            False, _pair(0), groups, bias, padding_mode, **factory_kwargs)
        self.freeze_weight = freeze_weight
        if self.freeze_weight:
            self.weight.requires_grad = False

        # Jit compilation method for cpp extention
        # set PyInit_ prefix to comply with the python module name
        self.nemo = nemo
        if self.nemo:
            self.act_bit = 9
            self.weight_bit = 9
            self.bias_bit= 9
            self.axx_conv2d_kernel = load(name='PyInit_conv2d_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-9x9/axx_conv2d.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        else:
            self.max_val = 127
            self.axx_conv2d_kernel = load(name='PyInit_conv2d_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-8x8/axx_conv2d.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)

        if groups != 1 and groups != in_channels:
            raise ValueError('AdaPT_Conv2d does not support groups != in_channels')
        self.data_shape = None

    def update_kernel(self):
        if self.nemo:
            self.axx_conv2d_kernel = load(name='PyInit_conv2d_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-9x9/axx_conv2d.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        else:
            self.axx_conv2d_kernel = load(name='PyInit_conv2d_' + self.axx_mult, sources=["./neural_networks/adapt/cpu-kernels-8x8/axx_conv2d.cpp"], extra_cflags=['-DAXX_MULT=' + self.axx_mult + ' -march=native -fopenmp -O3'], extra_ldflags=['-lgomp'], verbose=True)
        if self.freeze_weight:
            self.weight.requires_grad = False

    def _conv_forward(self, input: Tensor, weight: Tensor, bias: Optional[Tensor]):
        if self.nemo:
            return AdaPT_Conv2d_Function_nemo.apply(input, weight, self.kernel_size, self.out_channels, self.bias_, 
                                                    self.axx_conv2d_kernel, bias, self.stride, self.padding, 
                                                    self.dilation, self.groups, self.padding_mode, 
                                                    self.act_bit, self.weight_bit, self.bias_bit)
        return AdaPT_Conv2d_Function.apply(input, weight, self.kernel_size, self.out_channels, self.bias_,
                                           self.axx_conv2d_kernel, bias, self.stride, self.padding,
                                           self.dilation, self.groups, self.max_val)

    def forward(self, input: Tensor) -> Tensor:
        out = self._conv_forward(input, self.weight, self.bias)
        self.data_shape = out.shape
        return out

