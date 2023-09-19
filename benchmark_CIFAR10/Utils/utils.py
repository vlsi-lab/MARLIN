import os
import torch
from torch import nn
# import nemo
import numpy as np
# from torch_neural_networks_library import *
import json
# import onnx
# import netron
from packaging import version
# from Utils.NEMO_node import NEMO_node
import sys
# sys.path.append('../adapt')
from neural_networks.adapt.approx_layers import axx_layers as approxNN
from neural_networks.CIFAR10.resnet import ResidualModule


# TODO: pulire, eliminare doppioni, mergiare
sys.path.append('../adapt')
# from tools.LUT_convert import generate_mult_LUT
from pathlib import Path
"""
function to create the approximation dictionary

The dictionary will have this structure 

{ 
    'Conv1' : { 'approx_mul':1,
                'approx_add':0,
                'mul_conf':72,
                'add_conf' : 0
            }, 
    'act1'.....

}
There can be layers which are not approximate, such as maxpool
or relu or BN...
"""


def create_approximation_dict(model):
    model.eval()
    approximation_dict = {}
    for n,m in model._modules.items():
        approximation_dict[n]=dict()
        approximation_dict[n]['approx_mul'] = 0
        approximation_dict[n]['approx_mac'] = 0
        approximation_dict[n]['approx_dot8'] = 0
        approximation_dict[n]['mul_conf'] = 0
        #approximation_dict[n]['add_conf'] = 0
    return approximation_dict


def adapt_model_9x9(model, chromosome, axx_linear=False, retrain_type='bias'):
    """
    Assuming only 9x9 multipliers are used, choose configuration using chromosome
    """
    cnt = 0
    # act_bit = 8
    # weight_bit = 8
    # bias_bit = 8
    with torch.no_grad():
        for name,module in model._modules.items():
            if isinstance(module, nn.Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d):
                axx_mult = 'bw_mult_9_9_' + str(int(chromosome[cnt]))
                module.axx_mult = axx_mult
                if retrain_type == 'bias':
                    module.freeze_weight = True
                elif retrain_type == "full":
                    module.axx_mult = axx_mult
                    module.freeze_weight = False
                else:
                    module.axx_mult = axx_mult

                module.update_kernel()
                cnt += 1

            elif isinstance(module, nn.Sequential):
                for sub_name, sub_module in module._modules.items():
                    if isinstance(sub_module, ResidualModule):
                        for subsub_name, subsub_module in sub_module._modules.items():
                            if isinstance(subsub_module, nn.Conv2d) or isinstance(subsub_module, approxNN.AdaPT_Conv2d):
                                axx_mult = 'bw_mult_9_9_' + str(int(chromosome[cnt]))
                                subsub_module.axx_mult = axx_mult
                                if retrain_type == 'bias':
                                    subsub_module.freeze_weight = True
                                elif retrain_type == "full":
                                    subsub_module.axx_mult = axx_mult
                                    subsub_module.freeze_weight = False
                                else:
                                    subsub_module.axx_mult = axx_mult

                                subsub_module.update_kernel()
                                cnt += 1

            if axx_linear:
                if isinstance(module, nn.Linear) or isinstance(module, approxNN.AdaPT_Linear):
                    axx_mult = 'bw_mult_9_9_' + str(int(chromosome[cnt]))
                    module.axx_mult = axx_mult
                    if retrain_type == 'bias':
                        module.freeze_weight = True
                        module.freeze_bias = False
                    elif retrain_type == "full":
                        module.freeze_weight = False
                        module.freeze_bias = False
                    else:
                        module.freeze_bias = False
                    module.update_kernel()


#

def test(dataloader, model, loss_fn, device, integer = False):
    size = len(dataloader.dataset)
    num_batches = len(dataloader)
    model.eval()
    test_loss, correct = 0, 0
    with torch.no_grad():
        for X, y in dataloader:
            X = torch.round(X/torch.max(torch.abs(X)))
            if (integer == True):
                X = torch.round(X*255)
                torch.round(X)
            X, y = X.to(device), y.to(device)
            pred = model(X)
            test_loss += loss_fn(pred, y).item()
            correct += (pred.argmax(1) == y).type(torch.float).sum().item()
    test_loss /= num_batches
    correct /= size
    if (integer == True):
        print(f"Test Error: \n Accuracy: {(100*correct):>0.1f}% \n")
    else:
        print(f"Test Error: \n Accuracy: {(100*correct):>0.1f}%, Avg loss: {test_loss:>8f} \n")
    return correct

def print_acc(test_data, test_dataloader, model, device, integer = False):
    classes = test_data.classes
    correct_pred = {classname: 0 for classname in classes}
    total_pred = {classname: 0 for classname in classes}
    with torch.no_grad():
        for X, y in test_dataloader:
            X = torch.round(X/torch.max(torch.abs(X)))
            if (integer == True):
                X = torch.round(X*255)
                #X=int(X*255)
            images, labels = X.to(device), y.to(device)
            outputs = model(images)
            _, predictions = torch.max(outputs, 1)
            for label, prediction in zip(labels, predictions):
                if label == prediction:
                    correct_pred[classes[label]] += 1
                total_pred[classes[label]] += 1

    min_correct = [0,110]
    for classname, correct_count in correct_pred.items():
        accuracy = 100 * float(correct_count) / total_pred[classname]
        if min_correct[1] >= int(accuracy):
            min_correct = [classname, accuracy]
        print("Accuracy for class {:5s} is: {:.1f} %".format(classname, accuracy))

    lowest_class_accuracy = min_correct[1]
    print("Worst class accuracy is %.4f for class %s" %(min_correct[1], min_correct[0]))


def get_activation(name):
    activation = {}
    def hook(model, input, output):
        activation[name] = output.detach()
    return hook


