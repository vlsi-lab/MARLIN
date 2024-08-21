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

import argparse

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--number', default=0, type=int, help="Image class for MNIST dataset: must be a number in the range [0:9].")
    parser.add_argument('--dory-folder', default="./riscv_characterization/dory_inputs_generation/dory_folder/", type=str, help="String indicating the path where to store results produced.")
    
    return parser.parse_args()


def main():
    params = get_args()

    threads = 12
    torch.set_num_threads(threads)

    transform_test = transforms.Compose([transforms.ToTensor(), transforms.Normalize(0.1306, 0.308), transforms.Normalize(-0.5,1)])
    test_data = datasets.MNIST(root="data", train=False, download=True, transform=transform_test)

    batch_size = 32
    best_workers = 6

    test_dataloader = DataLoader(test_data, batch_size=batch_size, shuffle=False, num_workers=best_workers, pin_memory=False)

    #device = "cuda" if torch.cuda.is_available() else "cpu"
    device = 'cpu'
    print("Using {} device".format(device))

    model = MNIST_model1()  # create model instance, initialize parameters, send to device
    print(model)
    model.to(device)

    model_name = './riscv_characterization/nemo_training_quantization/saved_models_nemo/new_wd_model'
    model.load_state_dict(torch.load( model_name + '.pth')['model_state_dict'], strict=False)
    json_file='./riscv_characterization/nemo_training_quantization/act.json'
    nemo_utils.to_nemo_id_model(model, json_file)

    # read file produced by NSGA2 containing pareto optimal configurations
    # input selection
    in_t_all = nemo_utils.select_input(test_data, test_dataloader, device)   #return a list with an input for each classification element

    #name_file, number, dory_folder = sys.argv
    #if (len(sys.argv) < 3):
    #    print("Not enough argument passed to ", name_file)
    #    sys.exit()
    #number = int(number)
    # use input arguments
    if (params.number > 9 or params.number < 0):
        print("Error: please pass a number between 0 and 9")
        sys.exit()
    in_t = in_t_all[params.number]  #image with label 0

    # re-create onnx graph
    Path('./riscv_characterization/dory_inputs_generation/onnx').mkdir(parents=True, exist_ok=True)
    onnx_id_name = './riscv_characterization/dory_inputs_generation/onnx/prova_main_1.onnx'
    # Save and modify onnx model
    nemo.utils.export_onnx( onnx_id_name, model, model, (1,28,28),device)
    # use compliant format with respect to DORY parsing order
    onnx_model = onnx.load( onnx_id_name)
    nemo_utils.rename_edges_onnx(onnx_model, onnx_id_name)
    onnx_model = onnx.load(onnx_id_name)
    # write weights and activations values to be used by DORY to check correctness
    #dory_folder  = './dory_inputs_generation/'+ params.dory_folder 
    Path(params.dory_folder).mkdir(parents=True, exist_ok=True)
    nemo_utils.write_weights(model, params.dory_folder)
    nemo_utils.write_intermidiate_results(model, in_t, params.dory_folder)

if __name__ == "__main__":
    main()
