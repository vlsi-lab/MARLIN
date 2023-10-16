import torch
import torch.nn as nn
import numpy as np
import random
from neural_networks.adapt.approx_layers import axx_layers as approxNN
from neural_networks.CIFAR10.resnet import ResidualModule
from neural_networks.utils import evaluate_test_accuracy
import riscv_characterization.vendor.nemo

# TODO: controllare dependencies e isinstance che ci sia nemo dappertutto
def encode_chromosome(model, n_appr_levels = 255, axx_linear=False):
    """
    Return the number of Conv layers in the model, that is the core info to encode chromosome
    """
    n_layers = 0
    for name, module in model._modules.items():
        # or isinstance(module, nemo.quant.pact.PACT_Conv2d)
        # if isinstance(module, nn.Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d) or isinstance(module, nemo.quant.pact.PACT_Conv2d):
        if isinstance(module, nn.Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d):
                n_layers += 1
        elif isinstance(module, nn.Sequential):
            for sub_name, sub_module in module._modules.items():
                if isinstance(sub_module, ResidualModule):
                    for subsub_name, subsub_module in sub_module._modules.items():
                        # if isinstance(subsub_module, nn.Conv2d) or isinstance(subsub_module, approxNN.AdaPT_Conv2d) or isinstance(module, nemo.quant.pact.PACT_Conv2d):
                        if isinstance(subsub_module, nn.Conv2d) or isinstance(subsub_module, approxNN.AdaPT_Conv2d):
                            n_layers += 1
        if axx_linear:
            if isinstance(module, nn.Linear) or isinstance(module, approxNN.AdaPT_Linear):
                n_layers += 1

    return n_appr_levels, n_layers

def initialize_population(Np, n_levels, n_layers):
    """
    Randomly defines the initial population, with a number of elements Np, and returns it.
    Each chromosome is a list of n_layers element, each between 0 and n_levels-1
    """
    population = [] #list of lists, Np lists(chrom)
    for iteration in range(0, Np):
        chromosome = []
        for idx in range(0, n_layers):
            gene = np.uint8(random.randrange(0,n_levels))
            chromosome.append(gene)
        population.append(chromosome)
    return population


def list_mult_per_layer(model, imsize=(1,3,32,32), axx_linear=False):
    """
    @param model: torch.nn.module
    @param imsize: tuple
    @return:
    """
    model_mult = []
    in_t = (torch.randint(low=0, high=256, size=imsize, dtype=torch.float32))
    in_t.to('cpu')
    model.eval()
    with torch.no_grad():
        out = model(in_t)
    for name, module in model._modules.items():
        # if isinstance(module, nn.Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d) or isinstance(module, nemo.quant.pact.PACT_Conv2d):
        if isinstance(module, nn.Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d):
            wgt = module.weight.shape
            act = module.data_shape
            mult = wgt[0] * wgt[1] * wgt[2] * wgt[3] * act[2] * act[3]
            model_mult.append(mult)

        elif isinstance(module, nn.Sequential):
            for sub_name, sub_module in module._modules.items():
                if isinstance(sub_module, ResidualModule):
                    for subsub_name, subsub_module in sub_module._modules.items():
                        # if isinstance(subsub_module, nn.Conv2d) or isinstance(subsub_module, approxNN.AdaPT_Conv2d) or isinstance(module, nemo.quant.pact.PACT_Conv2d):
                        if isinstance(subsub_module, nn.Conv2d) or isinstance(subsub_module, approxNN.AdaPT_Conv2d):
                            wgt = subsub_module.weight.shape
                            act = subsub_module.data_shape
                            mult = wgt[0] * wgt[1] * wgt[2] * wgt[3] * act[2] * act[3]
                            model_mult.append(mult)
        if axx_linear:
            if isinstance(module, nn.Linear) or isinstance(module, approxNN.AdaPT_Linear):
                wgt = module.weight.shape
                mult = wgt[0] * wgt[1]
                model_mult.append(mult)

    return model_mult


def evaluate_accuracy(model, train_data, test_data, train_args, test_only=False, partial=False, partial_val=10, nemo=False, json_file=None):
    """
    Simulate the network with the given configuration and evaluate its accuracy on the test dataset.
    At each call the model is re-loaded from the beginning
    """
    if not test_only or not nemo:
        loss_fn = nn.CrossEntropyLoss()  # result is the same using this or CrossEntropy if softmax is in the model
        model.train()
        opt = torch.optim.SGD(model.parameters(), lr=train_args['lr_max'], weight_decay=train_args['weight_decay'], momentum=train_args['lr_momentum'])
        lr_steps = int(len(train_data)/100.0*partial_val)
        scheduler = torch.optim.lr_scheduler.CyclicLR(opt, base_lr=train_args['lr_min'], max_lr=train_args['lr_max'], step_size_up=lr_steps / 2, step_size_down=lr_steps / 2)

        for epoch in range(train_args['epochs']):
            train_loss = 0
            train_acc = 0
            train_n = 0

            index = np.random.randint(0, len(train_data)-int(len(train_data)/100.0*partial_val))

            for i, (X, y) in enumerate(train_data):
                if partial and i >= index + int(len(train_data)/100.0*partial_val):
                    break
                if partial and i < index:
                    continue
                X, y = X.to('cpu'), y.to('cpu')
                output = model(X)
                loss = loss_fn(output, y)
                opt.zero_grad()
                loss.backward()
                opt.step()
                scheduler.step()
                train_loss += loss.item() * y.size(0)
                train_acc += (output.max(1)[1] == y).sum().item()
                train_n += y.size(0)

    test_loss, test_acc = evaluate_test_accuracy(test_data, model, 'cpu')

    return test_acc


def mult_power_list(file_name):
    """
    Read file containing normalized power and returns a list with power for each approx level 
    """
    if file_name == None:
        file_name = 'mult_energy.txt'
    power_list = []
    f = open(file_name, 'r')
    while True:
        line = f.readline() # APPROX line
        if not line:
            break
        line = f.readline() # power line
        if not line:
            break
        dyn_power = line.replace('\n','')
        power_list.append(float(dyn_power))
    return power_list

def max_power_net(mult_per_layer, power_list):
    power = 0
    for n_mult in mult_per_layer:
        power += n_mult * power_list[0] ## CHECK: considero appr=0 come refernce
    return power

def evaluate_power(mult_per_layer, power_list, max_power, chromosome):
    """
    Find the average power consumed by the network in the current configuration.
    mult_per_layer : list containing the # of multiplication per each Conv layer
    power_list : list of the power consumed by each multiplier configuration [0:255]
    chromosome : approx_level of multiplier for each Conv layer
    """
    power = 0
    for i, n_mult in enumerate(mult_per_layer):
        power_mult = power_list[int(chromosome[i])]  
        power += power_mult * n_mult    # avg power of layer

    norm_power = power/max_power
    return norm_power

