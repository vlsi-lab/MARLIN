import os
import torch
from torch import nn
import riscv_characterization.vendor.nemo as nemo
from riscv_characterization.vendor.nemo.quant.pact import *
import numpy as np
import json
import onnx
import netron
import sys
from pathlib import Path
from packaging import version
from riscv_characterization.nemo_training_quantization import NEMO_node
from neural_networks.adapt.approx_layers import axx_layers as approxNN
from approximate_multiplier.LUT_convert import generate_mult_LUT


activation={}
def get_activation(name):
    def hook(model, input, output):
        activation[name] = output.detach()
    return hook


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

def fill_approximation_dict(model, chromosome):
    model.eval()
    approximation_dict = create_approximation_dict(model)
    idx = 0
    for name, module in model._modules.items():
        if isinstance (module, nn.Conv2d) or isinstance(module, PACT_Conv2d) or isinstance (module,approxNN.AdaPT_Conv2d):
            approximation_dict[name]['approx_mul'] = 1
            approximation_dict[name]['approx_mac'] = 1
            approximation_dict[name]['approx_dot8'] = 1
            approximation_dict[name]['mul_conf'] = chromosome[idx]
            idx +=1
        elif isinstance (module, nn.Linear) or isinstance(module, PACT_Linear):
            approximation_dict[name]['approx_mul'] = 1
            approximation_dict[name]['approx_mac'] = 1
            approximation_dict[name]['approx_dot8'] = 1
            approximation_dict[name]['mul_conf'] = 0

    return approximation_dict    


def create_nemo_graph(onnx_model, nemo_model, precision_dict, approximation_dict):
    nemo_model.eval()
    NEMO_graph = []
    print ("Creating NEMO graph")
    for node_iterating in onnx_model.graph.node :
        nemo_node = NEMO_node()
        nemo_node.populate_NEMO_node(node_iterating) 
        nemo_node.add_node_precision(node_iterating, precision_dict, nemo_model, NEMO_graph)
        nemo_node.add_node_approximation(node_iterating, approximation_dict, nemo_model)
        NEMO_graph.append(nemo_node)
    return NEMO_graph

def print_json_from_NEMO_graph(name_file, json_folder, NEMO_graph) :
    if json_folder is None:
        json_folder = 'json_files'
    os.system('mkdir -p {} '.format(json_folder)) 
    dict_graph = {}
    dict_graph["graph"] = []
    for i,nodes in enumerate(NEMO_graph):
        dict_graph["graph"].append(nodes.export_to_dict()) 
    with open("{}/{}.json".format(json_folder, name_file), "w") as outfile:
        json.dump(dict_graph, outfile, indent=2)
    print("Creating {}.json in {}/". format(name_file, json_folder))


# define a function which transforms the model into an adapt compatible one
def adapt_model(model, NEMO_graph):    
    model.eval()
    print("Transforming id model to adapt model")
    ##N.B. !!!!!!!!!!!!weight_bit and act_bit HAVE to correspond to axx_mult values
    for name,module in model._modules.items():
        #if isinstance(module, PACT_Conv2d):
        if isinstance(module, nn.Conv2d) or isinstance(module, PACT_Conv2d):
            for node in NEMO_graph:
                if(node.name.find(name)!= -1):
                    print("Found layer", name)
                    weight_bit = node.weight_bits +2 ## CHECK: ATTENZIONE 
                    act_bit = node.input_activation_bits +1 ## CHECK: ATTENZIONE 
                    bias_bit = node.bias_bits
                    approx = node.mul_conf
                    axx_mult = 'bw_mult_'+str(weight_bit)+"_"+str(act_bit)+"_"+str(approx)
                    axx_path = Path('./adapt/cpu-kernels/axx_mults/'+ axx_mult +'.h')  
                    if axx_path.is_file() is False:
                        generate_mult_LUT(weight_bit, act_bit, approx)
                    adapt_layer = approxNN.AdaPT_Conv2d(in_channels = module.in_channels, out_channels=module.out_channels, kernel_size=module.kernel_size, padding=module.padding, stride=module.stride, dilation=module.dilation, groups=module.groups,bias=True if module.bias is not None else False, axx_mult = axx_mult, nemo=True)
                    adapt_layer.weight = module.weight
                    adapt_layer.bias = module.bias
                    model._modules[name]= adapt_layer


def adapt_model_9x9(model, chromosome):
    """
    Assuming only 9x9 multipliers are used, choose configuration using chromosome
    """
    model.eval()
    cnt = 0
    act_bit = 9 
    weight_bit = 9
    #bias_bit = 32
    for name,module in model._modules.items():
        if isinstance(module, nn.Conv2d) or isinstance(module, PACT_Conv2d) or isinstance(module, approxNN.AdaPT_Conv2d):
            axx_mult = 'bw_mult_9_9_' + str(int(chromosome[cnt]))    
            print("Found layer", name, "mult:", axx_mult)
            axx_path = Path('./neural_networks/adapt/cpu-kernels-9x9/axx_mults_9x9/'+ axx_mult +'.h')  
            if axx_path.is_file() is False:
                generate_mult_LUT(weight_bit, act_bit, int(chromosome[cnt]))
            
            adapt_layer = approxNN.AdaPT_Conv2d(in_channels = module.in_channels, out_channels=module.out_channels, kernel_size=module.kernel_size, padding=module.padding, stride=module.stride, dilation=module.dilation, groups=module.groups,bias=True if module.bias is not None else False, axx_mult = axx_mult, nemo=True)
            adapt_layer.weight = module.weight
            adapt_layer.bias = module.bias
            model._modules[name]= adapt_layer
            cnt += 1

    
def write_act_parameters(nemo_model, json_file):
    print("Writing activations parameters")
    nemo_model.eval()
    activation_dict = {}
    if json_file is None:
        json_file = './nemo_training_quantization/act.json'
    for name, module in nemo_model._modules.items():     # assume no submodules
        if isinstance (module, PACT_IntegerAct):
            activation_dict[name] = dict()
            activation_dict[name]['eps_in'] = module.eps_in.item()
            activation_dict[name]['eps_out'] = module.eps_out.item()
            activation_dict[name]['D'] = module.D.item()
            activation_dict[name]['alpha_out'] = module.alpha_out
    with open(json_file, "w") as outfile:
        json.dump(activation_dict, outfile, indent=2)
    print("Creating {}.json in {}/". format(json_file, "json_files"))

def to_nemo_id_model(model, json_file):
# substitute to Relu layers PACT_IntegerAct layers, loading parameters from json file
    print("Transforming to nemo compatible id model")
    f = open(json_file)
    act_json = json.load(f)
    model.eval() 
    for name, module in model._modules.items():
        if (isinstance(module, nn.ReLU)):
            print ("Found layer", name)
            try:
                nemo_act = PACT_IntegerAct(eps_in=torch.tensor(act_json[name]['eps_in']) ,eps_out= torch.tensor(act_json[name]['eps_out']) , alpha=torch.ones(1))
                nemo_act.D = torch.tensor(act_json[name]['D'] )
                nemo_act.alpha_out = act_json[name]['alpha_out']
                model._modules[name] = nemo_act
            except:
                print("layer name not found in json file")

def transform_model(model, model_file, json_file, chromosome):
    """
    1. Reload initial fp weights
    2. Transform to NEMO compatible model (load act scaling factors)
    3. Transform to adapt model with "chromosome" approx levels
    """
    if model_file == None:
        model_file = "./riscv_characterization/NSGA2/mnist_model.pth" # default name
    model.load_state_dict(torch.load(model_file)['model_state_dict'], strict=False) 
    if json_file == None:
        json_file = './riscv_characterization/act.json'  # choose a default name
    to_nemo_id_model(model, json_file)
    adapt_model_9x9(model, chromosome)  # model becomes adapt model
                

def select_input(test_data,test_dataloader, device):
    classes = test_data.classes
    correct_pred = {classname: 0 for classname in classes}
    total_pred = {classname: 0 for classname in classes}
    class_el = [0,1,2,3,4,5,6,7,8,9]
    i = 0
    in_t_all =[]
    for X, y in test_dataloader:
        X = torch.round(X/torch.max(torch.abs(X)))
        X = torch.round(X*255)
        
        images, labels = X.to(device), y.to(device)
        for image, label in zip(images,labels):
            if label == class_el[i]:                            
                i += 1
                image = image[None, :]
                #print(image.size())
                in_t_all.append((image))
            if i == len(class_el):
                break
        if i == len(class_el):
             break
    return in_t_all

def write_intermidiate_results(model,in_t, ex_dir):
    # generate a random input tensor of the correct size, for now quantized between 0 and 255, uint 8(then we will move to int 8)
    print("Writing intermidiate results to file")
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    if in_t is None:
        in_t = (torch.randint(low=0, high=256, size=(1,1,28,28), dtype=torch.float32)).to(device)
    my_dir=ex_dir
    model.eval()
    
    act_nodes_idx = []
    pool_nodes_idx = []
    out_node_idx = []
    i = 0
    # for now get hooks only for act and MaxPool. There could be other cases more complex to add
    for name, module in model._modules.items():
        if isinstance(module, PACT_IntegerAct):
            model._modules[name].register_forward_hook(get_activation(str(name))) 
            act_nodes_idx.append([i, str(name)])
            i+=1
        elif isinstance(module, torch.nn.MaxPool2d):
            model._modules[name].register_forward_hook(get_activation(str(name))) 
            pool_nodes_idx.append([i, str(name)])
            i+=1
    #create hook for the output
    all_modules = list(model._modules)
    last_layer = all_modules[-1]
    model._modules[last_layer].register_forward_hook(get_activation(str(last_layer)))
    out_node_idx.append([i, str(last_layer)])    #last node idx
    
    print(act_nodes_idx)
    print(pool_nodes_idx)
    print(out_node_idx)

    with torch.no_grad():
        out = model(in_t)
    # X stores the correct input we wanted to read, print into txt file
    fname_in = my_dir + '/input.txt'
    f_in = open(fname_in, 'w')
    my_in = in_t[0].cpu().detach().numpy()
    size = my_in.shape
    my_string = "# input" + "( shape ["+str(size[0]) +', ' + str(size[1])  +', ' + str(size[2])  +"]),\n"
    f_in.write(my_string)
    for i in range(0,size[0]):
        for j in range(0,size[1]):
            for k in range(0,size[2]):
                val = f'{my_in[i][j][k]:.3f}'
                val = val + ','+'\n'
                f_in.write(val)
    f_in.close()

    # write a file for each layer numbered as Dory
    ## write act outputs
    for idx in range(len(act_nodes_idx)):
        f_name = my_dir + '/out_layer' + str(act_nodes_idx[idx][0]) +'.txt'
        my_act_str = act_nodes_idx[idx][1]
        my_act_val = activation[my_act_str][0].cpu().detach().numpy()
        my_act_size = my_act_val.shape
        f = open(f_name, 'w')
        my_string = "# layer" + str(act_nodes_idx[idx][0]) + '.relu' + str(idx) + "( shape [" + str(my_act_size[0]) + ', ' + str(my_act_size[1]) + ', ' + str(my_act_size[2]) + "]),\n"
        f.write(my_string)

        for i in range(0, my_act_size[1]):
            for j in range(0, my_act_size[2]):
                for k in range(0, my_act_size[0]):
                    val = f'{my_act_val[k][i][j]:.3f}'
                    val = val + ',\n'
                    f.write(val)
        f.close()
    ## write max pool outputs
    for idx in range(len(pool_nodes_idx)):   #one for each max pool layer
        f_name = my_dir +'/out_layer' + str(pool_nodes_idx[idx][0]) +'.txt'
        my_pool_str = pool_nodes_idx[idx][1]
        my_pool_val = activation[my_pool_str][0].cpu().detach().numpy()
        my_pool_size = my_pool_val.shape
        f = open(f_name, 'w')
        my_string = "# layer" + str(pool_nodes_idx[idx][0]) + '.maxpool' + str(idx) + "( shape [" + str(my_pool_size[0]) + ', ' + str(my_pool_size[1]) + ', ' + str(my_pool_size[2]) + "]),\n"
        f.write(my_string)

        for i in range(0, my_pool_size[1]):
            for j in range(0, my_pool_size[2]):
                for k in range(0, my_pool_size[0]):
                    val = f'{my_pool_val[k][i][j]:.3f}'
                    val = val + ',\n'
                    f.write(val)
        f.close()
    ## write final output
    for idx in range(len(out_node_idx)):  # one for each fc layer
        f_name = my_dir + '/out_layer' + str(out_node_idx[idx][0]) + '.txt'
        my_fc_str = out_node_idx[idx][1]
        my_fc_val = activation[my_fc_str][0].cpu().detach().numpy()
        my_fc_size = my_fc_val.shape
        f = open(f_name, 'w')
        my_string = "# layer" + str(out_node_idx[idx][0]) + '.fc' + str(idx) + "( shape [" + str(my_fc_size[0]) + "]),\n"
        f.write(my_string)

        for i in range(0, my_fc_size[0]):
            val = f'{my_fc_val[i]:.3f}'
            val = val + ',\n'
            f.write(val)
        f.close()

def write_weights(model, my_dir):
    print("Writing weights to file")
    #model_names = list(model._modules)
    #for i, name in enumerate(model_names):
        #print("name", model._modules[name], name, i)
        #if isinstance(model._modules[name], PACT_IntegerAct):
        #    print (layer.weight)
    model.eval()
    idx = 0
    for layer in model.children():
        #print("layer:", layer)
        if isinstance(layer, PACT_Conv2d) or isinstance(layer, nn.Conv2d) or isinstance(layer,approxNN.AdaPT_Conv2d):
            fname = my_dir + "/Conv2d_weights" + str(idx) +".txt"
            f = open(fname, "w")
            mystr = "CHECK L2 weights\n"
            idx+=1

            weights = layer.weight.cpu().detach().numpy()
            #if idx==1 :
            #    print(weights)
            #weights = weights.flatten('F')
            weights = weights.astype(np.uint8)
            f.write(mystr)
            weights = weights.astype(np.float32)
            #print(weights.flatten())
            for i in range(0,weights.shape[0]):
                for j in range(0,weights.shape[2]):
                    for k in range (0, weights.shape[3]):
                        for z in range(0,weights.shape[1]):
                            #print("w",weights[k][i][j])
                            myval=f'{weights[i][z][j][k]:.3f}'
                            myval = myval + ',\n'
                            f.write(myval)
            f.close()



def rename_edges_onnx(onnx_model, onnx_id_name):
    ## required for onnx == 1.8.1 in 1.12.0 gives problems
    # version 1.12.0 for node clips, defines min and max as inputs, and not only as attributes of the node

    print("Rename onnx edges")
    ## rename the inputs of the node which are at the root of the graph
    for idx, node in enumerate(onnx_model.graph.node):
        for i_idx, in_names in enumerate(node.input):
            if(in_names == onnx_model.graph.input[0].name):
                onnx_model.graph.node[idx].input[i_idx] = str(0)
    
    onnx_model.graph.input[0].name = str(0)
     
    for idx, node in enumerate(onnx_model.graph.node):
        # print("Look for", node.output)
        j = idx
        # change all inputs connected to this output
        for idx_2, node_2 in enumerate(onnx_model.graph.node):
            for i_idx,in_names in enumerate(node_2.input):
                if (in_names == node.output[0]):    #single output
                    onnx_model.graph.node[idx_2].input[i_idx] = str(idx+1)
        #if(onnx_model.graph.node[idx].op_type =='Clip'):
            #print(onnx_model.graph.node[idx])
            #retrieve value of min and max
        
    
        onnx_model.graph.node[idx].output[0] = str(idx+1)
    #rename only nodes going from an output to one input, not renaming internal nodes (ex. Mul constants, weigths ecc)
    onnx_model.graph.output[0].name = str(j+1)
    onnx.save(onnx_model,  onnx_id_name)


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
                if (torch.max(X)>255):
                    ValueError("max",torch.max(X))
                if torch.min(X)< 0:
                    ValueError("min", torch.min(X))
            X, y = X.to(device), y.to(device)
            pred = model(X)
            test_loss += loss_fn(pred, y).item()
            correct += (pred.argmax(1) == y).type(torch.float).sum().item()
    test_loss /= num_batches
    correct /= size
    if (integer == True):
        print(f"Accuracy: {(100*correct):>0.1f}% \n")
    else:
        print(f"Accuracy: {(100*correct):>0.1f}%, Avg loss: {test_loss:>8f} \n")
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


