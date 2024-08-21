import torch
from torch import nn
import torch.nn.functional as F
from neural_networks.adapt.approx_layers import axx_layers
from neural_networks.custom_layers import Conv2d, Linear

class adapt_mnist_net(nn.Module):
    def __init__(self):
        super(adapt_mnist_net, self).__init__()

        self.conv1 = axx_layers.AdaPT_Conv2d(in_channels=1, out_channels=3, kernel_size=(7, 7), stride=(1, 1),
                                             padding=(3, 3), groups=1, dilation=1, bias=False, axx_mult='bw_mult_9_9_0')
        self.relu1 = nn.ReLU()

        self.conv2 = axx_layers.AdaPT_Conv2d(in_channels=3, out_channels=8, kernel_size=(5, 5), stride=(1, 1),
                                             padding=(2, 2), groups=1, dilation=1, bias=False, axx_mult='bw_mult_9_9_0')
        self.relu2 = nn.ReLU()
        self.maxpool1 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(1,1))

        self.conv3 = axx_layers.AdaPT_Conv2d(in_channels=8, out_channels=10, kernel_size=(3, 3), stride=(1, 1),
                                             padding=(1, 1), groups=1, dilation=1, bias=False, axx_mult='bw_mult_9_9_0')
        self.relu3 = nn.ReLU()

        self.conv4 = axx_layers.AdaPT_Conv2d(in_channels=10, out_channels=16, kernel_size=(3, 3), stride=(1, 1),
                                             padding=(1, 1), groups=1, dilation=1, bias=False, axx_mult='bw_mult_9_9_0')
        self.relu4 = nn.ReLU()

        self.maxpool2 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(1,1))

        self.conv5 = axx_layers.AdaPT_Conv2d(in_channels=16, out_channels=24, kernel_size=(3, 3), stride=(1, 1),
                                             padding=(1, 1), groups=1, dilation=1, bias=False, axx_mult='bw_mult_9_9_0')
        self.relu5 = nn.ReLU()

        self.maxpool3 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(0,0))

        self.flatten = nn.Flatten()
        self.dense = axx_layers.AdaPT_Linear(9*24, 10, bias=False, axx_mult='bw_mult_9_9_0')

    def forward(self, inputs):
        out = self.relu1(self.conv1(inputs))
        out = self.maxpool1(self.relu2(self.conv2(out)))
        out = self.relu3(self.conv3(out))
        out = self.maxpool2(self.relu4(self.conv4(out)))
        out = self.maxpool3(self.relu5(self.conv5(out)))
        out = self.dense(out)

        return out


class quant_mnist_net(nn.Module):
    def __init__(self):
        super(quant_mnist_net, self).__init__()
        self.weight_bit = 8
        self.act_bit = 8
        self.bias_bit = 8

        self.conv1 = Conv2d(in_channels=1, out_channels=3, kernel_size=(7, 7), stride=(1, 1), padding=(3, 3), groups=1,
                            dilation=1, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                            bias_bit=self.bias_bit)
        self.relu1 = nn.ReLU()

        self.conv2 = Conv2d(in_channels=3, out_channels=8, kernel_size=(5, 5), stride=(1, 1), padding=(2, 2), groups=1,
                            dilation=1, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                            bias_bit=self.bias_bit)
        self.relu2 = nn.ReLU()
        self.maxpool1 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(1,1))

        self.conv3 = Conv2d(in_channels=8, out_channels=10, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1),
                            groups=1, dilation=1, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                            bias_bit=self.bias_bit)
        self.relu3 = nn.ReLU()

        self.conv4 = Conv2d(in_channels=10, out_channels=16, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1),
                            groups=1, dilation=1, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                            bias_bit=self.bias_bit)
        self.relu4 = nn.ReLU()
        self.maxpool2 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(1,1))

        self.conv5 = Conv2d(in_channels=16, out_channels=24, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1),
                            groups=1, dilation=1, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                            bias_bit=self.bias_bit)
        self.relu5 = nn.ReLU()
        self.maxpool3 = nn.MaxPool2d(kernel_size=(3, 3), stride=2, padding=(0,0))

        self.flatten = nn.Flatten()
        self.dense1 = Linear(9*24, 10, bias=False, act_bit=self.act_bit, weight_bit=self.weight_bit,
                             bias_bit=self.bias_bit)

    def forward(self, inputs):
        out = self.relu1(self.conv1(inputs))
        out = self.maxpool1(self.relu2(self.conv2(out)))
        out = self.relu3(self.conv3(out))
        out = self.maxpool2(self.relu4(self.conv4(out)))
        out = self.maxpool3(self.relu5(self.conv5(out)))
        out = self.dense(out)
        return out


# use either OUT_C_SIZE, IN_C_SIZE or F_SIZE that is a multiple of 4
class MNIST_model1(nn.Module):
    def __init__(self):
        super(MNIST_model1, self).__init__()

        self.Conv1 = nn.Conv2d(1, 4, 7, stride=1, padding=3, dilation=1, groups=1, bias=False)
        self.act1 = nn.ReLU()

        self.Conv2 = nn.Conv2d(4, 8, 5, stride=1, padding=2, dilation=1, groups=1, bias=False)
        self.act2 = nn.ReLU()
        self.Pool1 = nn.MaxPool2d(2, stride=2, padding=0, dilation=1)

        self.Conv3 = nn.Conv2d(8, 10, 3, stride=1, padding=1, dilation=1, groups=1, bias=False)
        self.act3 = nn.ReLU()

        self.Conv4 = nn.Conv2d(10, 16, 3, stride=1, padding=1, dilation=1, groups=1, bias=False)
        self.act4 = nn.ReLU()
        self.Pool2 = nn.MaxPool2d(2, stride=2, padding=0, dilation=1)

        self.Conv5 = nn.Conv2d(16, 24, 3, stride=1, padding=1, dilation=1, groups=1, bias=False)
        self.act5 = nn.ReLU()
        self.Pool3 = nn.MaxPool2d(2, stride=2, padding=0, dilation=1)

        self.flatten = nn.Flatten()
        self.linear1 = nn.Linear(24 * 3 * 3, 10, bias=False)

        torch.nn.init.kaiming_uniform_(self.linear1.weight, a=0, mode='fan_in', nonlinearity='relu')
        torch.nn.init.kaiming_uniform_(self.Conv1.weight, a=0, mode='fan_in', nonlinearity='relu')
        torch.nn.init.kaiming_uniform_(self.Conv2.weight, a=0, mode='fan_in', nonlinearity='relu')
        torch.nn.init.kaiming_uniform_(self.Conv3.weight, a=0, mode='fan_in', nonlinearity='relu')
        torch.nn.init.kaiming_uniform_(self.Conv4.weight, a=0, mode='fan_in', nonlinearity='relu')
        torch.nn.init.kaiming_uniform_(self.Conv5.weight, a=0, mode='fan_in', nonlinearity='relu')


    def forward(self, x):
        out = self.Conv1(x)
        out = self.act1(out)
        out = self.Conv2(out)
        out = self.act2(out)
        out = self.Pool1(out)
        out = self.Conv3(out)
        out = self.act3(out)
        out = self.Conv4(out)
        out = self.act4(out)
        out = self.Pool2(out)
        out = self.Conv5(out)
        out = self.act5(out)
        out = self.Pool3(out)
        out = self.flatten(out)
        out = self.linear1(out)
        out = F.log_softmax(out, dim=1)     #From NEMO tutorial: use the softmax, saying it does not need quantization, and use NLLloss
                                            # instead of cross entropy loss
        return out


precision_std = {
    'Conv1' : {
        'W_bits': 7     # corrisponde a 8 bit unsigned 0:255 (quindi dovrei fare +2?)
    },
    'act1' : {
        'x_bits' : 8    ## this is 8 bit unsigned so the clip will go from 0 to 255
    },
    'Conv2': {
        'W_bits': 7    
    },
    'act2' : {
        'x_bits' : 8    ## this is 8 bit unsigned so the clip will go from 0 to 255
    },
    'Conv3': {
        'W_bits': 7
    },
    'act3' : {
        'x_bits' : 8
    },
    'Conv4': {
        'W_bits': 7
    },
    'act4' : {
        'x_bits' : 8
    },
    'Conv5': {
        'W_bits': 7
    },
    'act5' : {
        'x_bits' : 8
    },
    'linear1' : {
        'W_bits' : 7
    },
}
