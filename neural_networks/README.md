# Neural networks
This folder contains the files to train or test the neural networks that are used in other parts of the MARLIN framework.

## Extract the pre-trained models
In order to replicate the results presented in the paper, you must use the pre-trained NN models inside "models.7z". Extract the content and place the .pth files inside the folder "models". After doing so, it is possible to test them using the training scripts detailed in the next sections.

## Setup AdaPT
This framework uses a modified version of AdaPT to accelerate approximate NNs. The permission to use the code has been granted by their developer. You can check out the [repository](https://github.com/dimdano/adapt/tree/main) and [paper](https://ieeexplore.ieee.org/abstract/document/9913212/) for more info on the implementation and how AdaPT simulates the execution of approximate NNs.

Before you can use this modified version of adapt with MARLIN, you must extract two archives inside "neural_networks/adapt/cpu-kernels-8x8" and "neural_networks/adapt/cpu-kernels-9x9". In particular, you must extract the content of "axx_mults_8x8.7z", which are 256 headers with the multiplier's LUT for each approximation level, and place them inside the folder "axx_mults_8x8". Then do the same for "axx_mults_9x9.7z" and place them inside "axx_mults_9x9".


## CIFAR-10
To train CIFAR-10 NNs you can use the script "train_cifar10.py", you can check the options by executing the following command: 

> python neural_networks/CIFAR10/train_cifar10.py --help

The default parameters are already set to reproduce the training results presented in the paper. You can check the test results for the six float and quantized ResNet models by executing the following command, substituting the correct layer number (8, 14, 20, 32, 50, 56) instead of XX:

> python neural_networks/CIFAR10/train_cifar10.py --neural-network resnetXX --execution-type float --reload True  # test float model
> 
> python neural_networks/CIFAR10/train_cifar10.py --neural-network resnetXX --execution-type quant --reload True  # test 8-bit quantized model

The ResNet models support the following execution type: "float", "quant", "adapt", "fused" and "fused-test".
"float" mode simply means that every parameter/activation is processed with floating point precision. "quant" mode uses a fake-quantize approach to simulate the effect of quantization. "adapt" mode simulates the network using LUTs that model the input/output of a real multiplier, useless for exact NNs, but necessary to evaluate approximate NNs. The usage of this mode is not recommended while training the model from scratch, as CPU training is 10x/20x slower than GPU training and the model performance would be the same (using the default multiplier, i.e., bw_mult_9_9_0 which has approximation level 0).
"fused" can be used to train the model while folding the batch normalization into the convolution. "fused-test" can be used to test ResNet models that were trained with the batch normalization folded.


## MNIST 
The training script used for MNIST is almost identical to the one used for CIFAR-10. Contrary to CIFAR-10, "fused" modes are not supported right now, because the provided NNs for MNIST are not using batch normalization. Notice that this implementation requires a different model for "quant" and "adapt" NNs and that float training is not supported. You can launch the training with:

>  python neural_networks/MNIST/train_mnist.py --neural-network-type adapt 

or
 
>  python neural_networks/MNIST/train_mnist.py --neural-network-type quant 

To see which options can be changed with respect to the default values, you can execute:
>  python neural_networks/MNIST/train_mnist.py --help
