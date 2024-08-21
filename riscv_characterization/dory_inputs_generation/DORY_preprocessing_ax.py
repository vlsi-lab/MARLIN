import torch
from torch import nn
from torch.utils.data import DataLoader
from torchvision import datasets
from torchvision.transforms import transforms
import riscv_characterization.vendor.nemo as nemo 
import numpy as np
from pathlib import Path
import json
import onnx
import re
from packaging import version
import sys
from riscv_characterization.nemo_training_quantization import utils as nemo_utils
from neural_networks.MNIST.mnist_net import MNIST_model1, precision_std
import argparse

"""
When called it requires some additional input arguments:
- number: a number between [0-9] which identifies the class to which the selected input image will belong
- list_idx: among the selected Pareto solutions, take the one at position list_idx
- dory_folder: name of the folder where to store required info for DORY pass
"""
def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--number', default=0, type=int, help="Image class for MNIST dataset: must be a number in the range [0:9].")
    parser.add_argument('--list-idx', default=0, type=int, help="Index of the approx solution to take among those selected from the Pareto front.")
    parser.add_argument('--dory-folder', default="./riscv_characterization/dory_inputs_generation/dory_folder/", type=str, help="String indicating the path where to store results produced.")
    
    return parser.parse_args()


def main():
    params = get_args()
    threads = 12
    torch.set_num_threads(threads)
    transform_test = transforms.Compose([transforms.ToTensor(), transforms.Normalize(0.1306, 0.308), transforms.Normalize(-0.5,1)])

    test_data = datasets.MNIST(root="data", train=False, download=True, transform=transform_test)

    batch_size = 32
    # best_workers = find_num_workers(training_data=training_data, batch_size=batch_size)
    best_workers = 6

    test_dataloader = DataLoader(test_data, batch_size=batch_size, shuffle=False, num_workers=best_workers, pin_memory=False)

    device = 'cpu'
    print("Using {} device".format(device))

    model = MNIST_model1()  # create model instance, initialize parameters, send to device
    model.to(device)

    model_name = './riscv_characterization/nemo_training_quantization/saved_models_nemo/new_wd_model'
    model.load_state_dict(torch.load( model_name + '.pth')['model_state_dict'], strict=False)
    json_file='./riscv_characterization/nemo_training_quantization/act.json'
    nemo_utils.to_nemo_id_model(model, json_file)

    # read file produced by NSGA2 containing pareto optimal configurations
    file_mult_conf = "./riscv_characterization/NSGA2/results_27s_ultra_txt/pareto_conf_80_50_Pc0.8_Pm0.8_seed1_singlepoint.txt"
    file_mult_res = "./riscv_characterization/NSGA2/results_27s_ultra_txt/pareto_res_80_50_Pc0.8_Pm0.8_seed1_singlepoint.txt"

    f1 = open(file_mult_conf, 'r')
    f2 = open(file_mult_res, 'r')
    # to find min and max, between 80% and 100% accuracy
    max_acc = 0.8
    min_acc = 1.0
    best_candidates = [] # list of all candidates fulfilling the max_acc, min_acc constraints
    while True:
        # file parsing
        line1 = f1.readline()
        line2 = f2.readline()
        if not line1:
            break
        if not line2:
            break
        line1 = line1.replace('\n', '') 
        line1 = line1.replace('[', '') 
        line1 = line1.replace(']', '') 
        line1 = re.sub(' +', ' ', line1)
        line1 = line1.split(' ')
        line1 = list(map(int, line1))
        
        line2 = line2.replace('\n', '') 
        line2 = line2.replace('[', '') 
        line2 = re.sub('\s*]',']', line2)
        line2 = line2.replace(']', '') 
        line2 = re.sub(' +', ' ', line2)
        line2 = line2.split(' ')
        line2 = list(map(float, line2))
        #line2[1] is accuracy of the model
        acc = 1.0/line2[1]
        # list of candidates with acc > 80%
        if (acc > 0.8):
            best_candidates.append(line1)
        ## find maximum
        #if (acc > max_acc): #keep only points with higher accuracy than 80%
        #    #find maximum
        #    max_acc = acc
        #    chromosome = line1 #store chromosome for current max accuracy
        # find minimum but above 80%
        if (acc < min_acc and acc > 0.8): #keep only points with higher accuracy than 80%
            #find maximum
            min_acc = acc
            min_chr = line1
            #chromosome = line1 #store chromosome for current min accuracy
        ## find a point in a specific range of accuracy
        #if (acc > 0.85 and acc < 0.9 ):
        #    chromosome = line1
        #    break
        #if (acc > 0.89 and acc < 0.92 ):
        #   chromosome = line1
        #   break
        #if (acc > 0.92 and acc < 0.95 ):
        #   chromosome = line1
        #   break

    print("Min chromosome:", min_chr, " with accuracy", min_acc)
    # input selection
    in_t_all = nemo_utils.select_input(test_data, test_dataloader, device)   # return a list with an input for each classification element

    # Parameters input by the user
    #name_file, number, list_idx, dory_folder = sys.argv
    #print(sys.argv)
    #if (len(sys.argv) < 4):
    #    print("Not enough argument passed to ", name_file)
    #    sys.exit()
    #number = int(number)
    #list_idx = int(list_idx)
    if (params.number > 9 or params.number < 0):
        print("Error: please pass a number between 0 and 9")
        sys.exit()
    if (params.list_idx < 0 or params.list_idx >len(best_candidates)-1 ):
        print("Error: list_idx out of range")
        sys.exit()
    print(params.number)
    # take the input value
    in_t = in_t_all[params.number]  #image with label number

    # take a single chromosome from the retrieved list
    #chromosome = best_candidates[params.list_idx]
    chromosome = [0, 0, 0, 0, 0]    # estimate power for all exact
    print("candidates:", best_candidates)
    print(chromosome)

    Path('./dory_inputs_generation/onnx').mkdir(parents=True, exist_ok=True)
    onnx_id_name = './dory_inputs_generation/onnx/prova_main_1.onnx'
    ##### Save and modify onnx model
    nemo.utils.export_onnx( onnx_id_name, model, model, (1,28,28),device)
    # use compliant format with respect to DORY parsing order
    onnx_model = onnx.load( onnx_id_name)
    nemo_utils.rename_edges_onnx(onnx_model, onnx_id_name)
    onnx_model = onnx.load(onnx_id_name)
    # create approximate dict with chromosome configuration
    approximation_dict = nemo_utils.fill_approximation_dict(model, chromosome)

    print(approximation_dict)
    ### save nemo graph in json format
    #dory_folder  = './dory_inputs_generation/'+ dory_folder 
    Path(params.dory_folder).mkdir(parents=True, exist_ok=True)
    json_graph = "dory_deployable_model"
    NEMO_graph = nemo_utils.create_nemo_graph(onnx_model, model, precision_std, approximation_dict)
    nemo_utils.print_json_from_NEMO_graph(json_graph, params.dory_folder, NEMO_graph)

    # make the model approximate
    nemo_utils.adapt_model(model, NEMO_graph)
    # write weigths and activations values to be used by DORY executable to check correctness
    nemo_utils.write_weights(model, params.dory_folder)
    nemo_utils.write_intermidiate_results(model, in_t, params.dory_folder)

if __name__ == "__main__":
    main()
