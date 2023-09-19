import torch
import torch.nn as nn
import torch.nn.functional as F
from neural_networks.adapt.approx_layers import axx_layers
from neural_networks.custom_layers import Conv2d, Linear, ReLu, FusedConvBN


class ResidualModule(nn.Module):
    ratio = 1
    act_bit = 8
    weight_bit = 8
    bias_bit = 8
    bn_momentum = 0.1

    def __init__(self, input_channels, output_channels, stride=1, mode=None):
        super(ResidualModule, self).__init__()

        if mode == 'fused' or mode == 'fused_test' or mode == 'fused_adapt':
            self.bn1 = nn.Identity()
        else:
            self.bn1 = nn.BatchNorm2d(input_channels, momentum=self.bn_momentum)

        if mode == 'float':
            self.conv1 = nn.Conv2d(input_channels, output_channels, kernel_size=3, stride=stride, padding=1, bias=False)
        elif mode == 'quant':
            self.conv1 = Conv2d(input_channels, output_channels, kernel_size=3, stride=stride, padding=1, bias=False,
                                act_bit=8, weight_bit=8, bias_bit=8)
        elif mode == 'adapt':
            self.conv1 = axx_layers.AdaPT_Conv2d(input_channels, output_channels, kernel_size=3, stride=stride,
                                                 padding=1, bias=False, axx_mult="bw_mult_9_9_0")
        elif mode == 'fused':
            self.conv1 = FusedConvBN(input_channels, output_channels, kernel_size=(3, 3), stride=stride, padding=1,
                                     bn_momentum=self.bn_momentum, freeze_norm=not self.training, act_bit=8,
                                     weight_bit=8, bias_bit=8)
        elif mode == 'fused_test':
            self.conv1 = Conv2d(input_channels, output_channels, kernel_size=3, stride=stride, padding=1, bias=True,
                                act_bit=8,
                                weight_bit=8, bias_bit=8)
        elif mode == 'fused_adapt':
            self.conv1 = axx_layers.AdaPT_Conv2d(input_channels, output_channels, kernel_size=3, stride=stride,
                                                 padding=1, bias=True,
                                                 axx_mult="bw_mult_9_9_0")
        else:
            exit("unknown layer command")

        self.relu1 = ReLu(act_bit=8)
        if mode == "adapt":
            self.relu1.alpha.requires_grad = True

        if mode == 'fused' or mode == 'fused_test':
            self.bn2 = nn.Identity()
        else:
            self.bn2 = nn.BatchNorm2d(output_channels, momentum=self.bn_momentum)

        if mode == 'float':
            self.conv2 = nn.Conv2d(output_channels, output_channels, kernel_size=3, stride=1, padding=1, bias=False)
        elif mode == 'quant':
            self.conv2 = Conv2d(output_channels, output_channels, kernel_size=3, stride=1, padding=1, bias=False,
                                act_bit=8, weight_bit=8,
                                bias_bit=8)
        elif mode == 'adapt':
            self.conv2 = axx_layers.AdaPT_Conv2d(output_channels, output_channels, kernel_size=3, stride=1, padding=1,
                                                 bias=False,
                                                 axx_mult="bw_mult_9_9_0")
        elif mode == 'fused':
            self.conv2 = FusedConvBN(output_channels, output_channels, kernel_size=(3, 3), stride=1, padding=1,
                                     bn_momentum=self.bn_momentum, freeze_norm=not self.training, act_bit=8,
                                     weight_bit=8, bias_bit=8)
        elif mode == 'fused_test':
            self.conv2 = Conv2d(output_channels, output_channels, kernel_size=3, stride=1, padding=1, bias=True,
                                act_bit=8, weight_bit=8,
                                bias_bit=8)
        else:
            exit("unknown layer command")

        self.relu2 = ReLu(act_bit=8)
        if mode == "adapt":
            self.relu2.alpha.requires_grad = True

        self.downsample = nn.AvgPool2d(kernel_size=1, stride=2)
        if stride != 1 or input_channels != self.ratio * output_channels:
            self.shortcut = True

    def shortcut_identity(self, prex, x):
        """
        Identity mapping implementation of the shortcut version without parameters.
        @param prex: activations tensor before the activation layer
        @param x: input tensor
        @return: sum of input tensors
        """
        if x.shape != prex.shape:
            d = self.downsample(x)
            p = torch.mul(d, 0)
            return prex + torch.cat((d, p), dim=1)
        else:
            return prex + x

    def forward(self, x):
        out = self.relu1(self.bn1(x))
        out = self.conv1(out)
        out = self.conv2(self.relu2(self.bn2(out)))
        out = self.shortcut_identity(out, x) if hasattr(self, 'shortcut') else out + x
        return out


class ResNet(nn.Module):
    def __init__(self, res_block, num_res_blocks, num_classes=10, mode=None):
        super(ResNet, self).__init__()
        self.input_channels = 16
        self.bn_momentum = 0.1
        if mode == 'float':
            self.conv1 = nn.Conv2d(3, self.input_channels, kernel_size=3, stride=1, padding=1, bias=False)
        elif mode == 'quant':
            self.conv1 = Conv2d(3, self.input_channels, kernel_size=3, stride=1, padding=1, bias=False, act_bit=8,
                                weight_bit=8, bias_bit=8)
        elif mode == 'adapt':
            self.conv1 = axx_layers.AdaPT_Conv2d(in_channels=3, out_channels=self.input_channels, kernel_size=(3, 3),
                                                 stride=(1, 1),
                                                 padding=(1, 1), groups=1, dilation=1, bias=False,
                                                 axx_mult="bw_mult_9_9_0")

        elif mode == 'fused':
            self.conv1 = FusedConvBN(in_channels=3, out_channels=self.input_channels, kernel_size=(3, 3), stride=1,
                                     padding=1, bn_momentum=self.bn_momentum, freeze_norm=not self.training, act_bit=8,
                                     weight_bit=8, bias_bit=8)
        elif mode == 'fused_test':
            self.conv1 = Conv2d(3, self.input_channels, kernel_size=3, stride=1, padding=1, bias=True, act_bit=8,
                                weight_bit=8, bias_bit=8)
        else:
            exit("unknown layer command")

        self.relu = ReLu(act_bit=8)
        if mode == "adapt":
            self.relu.alpha.requires_grad = True

        self.layer1 = self._make_res(res_block, 16, num_res_blocks[0], stride=1, mode=mode)
        self.layer2 = self._make_res(res_block, 32, num_res_blocks[1], stride=2, mode=mode)
        self.layer3 = self._make_res(res_block, 64, num_res_blocks[2], stride=2, mode=mode)
        if mode == 'fused' or mode == 'fused_test':
            self.bn = nn.Identity()
        else:
            self.bn = nn.BatchNorm2d(64 * res_block.ratio, momentum=self.bn_momentum)

        self.flatten = nn.Flatten()
        if mode == 'float':
            self.linear = nn.Linear(64 * res_block.ratio, num_classes)
        else:
            self.linear = Linear(64 * res_block.ratio, num_classes, act_bit=8, weight_bit=8, bias_bit=8)
        # elif mode == 'adapt':
        #     self.linear = axx_layers.AdaPT_Linear(64 * res_block.ratio, num_classes, bias=True, axx_mult="bw_mult_9_9_0")

    def _make_res(self, res_block, output_channels, num_res_blocks, stride, mode=None):
        strides = [stride] + [1] * (num_res_blocks - 1)
        layers = []
        for stride in strides:
            layers.append(res_block(self.input_channels, output_channels, stride, mode))
            self.input_channels = output_channels * res_block.ratio
        return nn.Sequential(*layers)

    def forward(self, x):
        out = self.conv1(x)
        out = self.layer1(out)
        out = self.layer2(out)
        out = self.layer3(out)
        out = self.relu(self.bn(out))
        out = F.avg_pool2d(out, 8)
        out = self.flatten(out)
        out = self.linear(out)
        return out


def resnet8(mode=None):
    return ResNet(ResidualModule, [1, 1, 1], mode=mode)


def resnet14(mode=None):
    return ResNet(ResidualModule, [2, 2, 2], mode=mode)


def resnet20(mode=None):
    return ResNet(ResidualModule, [3, 3, 3], mode=mode)


def resnet32(mode=None):
    return ResNet(ResidualModule, [5, 5, 5], mode=mode)


def resnet50(mode=None):
    return ResNet(ResidualModule, [8, 8, 8], mode=mode)


def resnet56(mode=None):
    return ResNet(ResidualModule, [9, 9, 9], mode=mode)

