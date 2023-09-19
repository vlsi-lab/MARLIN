from torch import nn
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


precision_std = {
    'Conv1' : {
        'W_bits': 7     # corrisponde a 8 bit unsigned 0:255 (quindi dovrei fare +2?)
    },
    'act1' : {
        'x_bits' : 8    ## this is 8 bit unsigned so the clip will go from 0 to 255
    },
    'Conv2': {
        'W_bits': 7     ## 5 bits signed quantization
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
