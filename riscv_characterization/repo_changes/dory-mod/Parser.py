# should work even without -*-
# -*- coding: utf-8 -*-
#!/bin/bash
# ONNX_management.py
# Alessio Burrello <alessio.burrello@unibo.it>
# Thorir Mar Ingolfsson <thoriri@iis.ee.ethz.ch>
#
# Copyright (C) 2019-2020 University of Bologna
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Libraries
import json
import os
import numpy as np
# DORY modules
from dory.Frontend_frameworks.NEMO.Pattern_rewriter import Pattern_rewriter
from dory.Parsers.Parser_ONNX_to_DORY import Parser_ONNX_to_DORY

# Directory
file_path = "/".join(os.path.realpath(__file__).split("/")[:-1])


class onnx_manager(Parser_ONNX_to_DORY):
    # Used to manage the ONNX files. By now, supported Convolutions (PW and DW), Pooling, Fully Connected and Relu.

    def __init__(self, onnx, config_file):
        layers_accepted = ['Conv', 'Pad', 'Mul', 'Add', 'Div', 'Constant', 'AveragePool', 'GlobalAveragePool', 'MaxPool', 'Cast', 'Clip', 'Floor', 'Flatten', 'Gemm', 'MatMul', 'Shape', 'Gather', 'Unsqueeze', 'Concat', 'Reshape', 'Sigmoid', 'LogSoftmax']
        layers_neglected = ['Cast', 'Floor', 'Flatten', 'Shape', 'Gather', 'Unsqueeze', 'Concat', 'Reshape', 'Sigmoid', 'LogSoftmax']
        layers_to_node = ['AveragePool', 'MaxPool', 'Conv', 'Gemm', 'MatMul', 'GlobalAveragePool']
        f = open(os.path.join(file_path, "rules.json"))
        rules = json.load(f)
        self.BNRelu_bits = config_file["BNRelu_bits"]
        try:
            self.n_test_inputs = config_file["n_inputs"]
        except KeyError:
            self.n_test_inputs = 1
        # add a config_file to set the additional info (in json formar)
        super().__init__(config_file, onnx, rules, layers_accepted, layers_neglected, layers_to_node)

    def frontend_mapping_to_DORY_nodes(self):
        print("\nNEMO Frontend: Matching patterns from generated ONNX to DORY.")
        for i, node in enumerate(self.DORY_Graph):
            node.add_existing_parameter('n_test_inputs', self.n_test_inputs)
            string_matching, indexes = self.pattern_matching(node, i)
            if isinstance(string_matching, str):
                self.DORY_Graph = Pattern_rewriter(self.DORY_Graph).execute(string_matching, indexes)
        print("\nNEMO Frontend: Updating Add nodes with constants.")
        for i, node in enumerate(self.DORY_Graph):
            if "Addition" in node.name:
                ## output parameters 
                node.outmul = {}
                node.outmul["value"] = 1
                node.outmul["layout"] = ""
                node.constant_names.append("outmul")
                node.outadd = {}
                node.outadd["value"] = 0
                node.outadd["layout"] = ""
                node.constant_names.append("outadd")
                # input 1 parameters
                node.inshift1 = {}
                node.inshift1["value"] = 0
                node.inshift1["layout"] = ""
                node.constant_names.append("inshift1")
                node.inadd1 = {}
                node.inadd1["value"] = 0
                node.inadd1["layout"] = ""
                node.constant_names.append("inadd1")
                # input 2 parameters
                node.inshift2 = {}
                node.inshift2["value"] = 0
                node.inshift2["layout"] = ""
                node.constant_names.append("inshift2")
                node.inadd2 = {}
                node.inadd2["value"] = 0
                node.inadd2["layout"] = ""
                node.constant_names.append("inadd2")
        
    def add_nodes_CSR_configuration(self):
        for i, node in enumerate(self.DORY_Graph):
            if (node.approx_mul or node.approx_mac or node.approx_dot8) :
                # dummy example, notice here we assume node.weight_bits is not None, or error, and it has its effective (not rounded) value
                mask = np.uint32(0)
                active_approx_hw = (node.approx_dot8 << 2) | (node.approx_mac << 1) | node.approx_mul
                mask = mask | active_approx_hw
                approx_mask = np.uint8(~(node.mul_conf))
                mask = mask |(approx_mask << 3)
                print(node.name, node.input_activation_bits, node.weight_bits)
                res_bits = node.input_activation_bits + node.weight_bits -4 #-4 as mask is on 14 bits, not 18
                res_mask = 2**(res_bits)-1
                mask = mask | (res_mask<<18)
                print("mask=", mask)
            #simple case, does not use approx_add or distinguish among node types
            else:
                mask = 0
            node.add_existing_parameter("CSR_approx",mask)
        
    def add_nodes_precision(self):
        print("\nNEMO Frontend: Adding Bit and Types to Nodes.")
        # Right now, the precision is fixed. We can extract it from either the original onnx graph or from a json.
        #if(self.nemo_json_file == None):
        for i, node in enumerate(self.DORY_Graph):
            if (node.weight_type is None):
                node.add_existing_parameter("weight_type", "int")
            if (node.constant_type is None):
                node.add_existing_parameter("constant_type", "int")
            if (node.output_activation_type is None and i!= (len(self.DORY_Graph)-1)):
                node.add_existing_parameter("output_activation_type", "uint")
            if i == len(self.DORY_Graph) -1:
                if (node.output_activation_type is None):
                    node.add_existing_parameter("output_activation_type", "int")
            if (node.input_activation_type is None):
                node.add_existing_parameter("input_activation_type", "uint")
            

            if node.name in ["Addition"]:
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
                if (node.second_input_activation_bits is None):
                    node.add_existing_parameter("second_input_activation_bits", 32)
                if (node.second_input_activation_type is None):
                    node.add_existing_parameter("second_input_activation_type", "uint")
            elif node.name in ["QAddition"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                # go to generic group
                #if (node.output_activation_bits is None):
                #    node.add_existing_parameter("output_activation_bits", 8)
                #if (node.input_activation_bits is None):
                #    node.add_existing_parameter("input_activation_bits", 8)
                if (node.second_input_activation_bits is None):
                    node.add_existing_parameter("second_input_activation_bits", 8)
                if (node.second_input_activation_type is None):
                    node.add_existing_parameter("second_input_activation_type", "uint")
            elif node.name in ["Relu", "BNRelu", "Clip"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                #if (node.output_activation_bits is None):
                #    node.add_existing_parameter("output_activation_bits", 8)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
            elif node.name in [ "Mul", "Add", "Div", "Shift"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
            elif node.name in ["Convolution", "FullyConnected"]:
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                try:
                    if(node.weight_bits <= 2):
                        node.weight_bits = 2
                    elif (node.weight_bits <= 4):
                        node.weight_bits = 4
                    elif (node.weight_bits <= 8):
                        node.weight_bits = 8
                except:
                    node.add_existing_parameter("weight_bits", 8) 
            # generic input bits rounding
            try:
                if(node.input_activation_bits <= 2):
                    node.input_activation_bits = 2
                elif (node.input_activation_bits <= 4):
                    node.input_activation_bits = 4
                elif (node.input_activation_bits <= 8):
                    node.input_activation_bits = 8
                # else remains at the same value, ex.32
            except:
                node.add_existing_parameter("input_activation_bits",8)
            # generic output bits rounding
            try:
                if(node.output_activation_bits <= 2):
                    node.output_activation_bits = 2
                elif (node.output_activation_bits <= 4):
                    node.output_activation_bits = 4
                elif (node.output_activation_bits <= 8):
                    node.output_activation_bits = 8
            except:
                node.add_existing_parameter("output_activation_bits",8)
            
            try:
                if(node.bias_bits <= 2):
                    node.bias_bits = 2
                elif (node.bias_bits <= 4):
                    node.bias_bits = 4
                elif (node.bias_bits <= 8):
                    node.bias_bits = 8
            except:
                if node.name in 'Convolution':
                    node.add_existing_parameter("bias_bits", 8)
                else:
                    node.add_existing_parameter("bias_bits", 32)        ;#NOTE: this can be a problem for linear layers different from the last one on 32 bits


            try:
                if(node.constant_bits <= 2):
                    node.constant_bits = 2
                elif (node.constant_bits <= 4):
                    node.constant_bits = 4
                elif (node.constant_bits <= 8):
                    node.constant_bits = 8
            except:
                node.add_existing_parameter("constant_bits", 8)
            # specific of Conv and FC layers
            # specific of Addition layer


            """
            if (node.bias_bits is None):
                node.add_existing_parameter("bias_bits", 32)
            if node.name in ["Convolution", "FullyConnected"]:
                if (node.weight_bits is None):
                    node.add_existing_parameter("weight_bits", 8)
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 8)
            elif node.name in ["Addition"]:
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
                if (node.second_input_activation_bits is None):
                    node.add_existing_parameter("second_input_activation_bits", 32)
                if (node.second_input_activation_type is None):
                    node.add_existing_parameter("second_input_activation_type", "uint")
            elif node.name in ["QAddition"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 8)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 8)
                if (node.second_input_activation_bits is None):
                    node.add_existing_parameter("second_input_activation_bits", 8)
                if (node.second_input_activation_type is None):
                    node.add_existing_parameter("second_input_activation_type", "uint")
            elif node.name in ["Pooling"]:
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 8)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 8)
            elif node.name in ["Relu", "BNRelu", "Clip"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 8)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
            elif node.name in [ "Mul", "Add", "Div", "Shift"]:
                if (node.constant_bits is None):
                    node.add_existing_parameter("constant_bits", self.BNRelu_bits)
                if (node.output_activation_bits is None):
                    node.add_existing_parameter("output_activation_bits", 32)
                if (node.input_activation_bits is None):
                    node.add_existing_parameter("input_activation_bits", 32)
        """
                
    def add_data_layout(self):
        print("\nNEMO Frontend: Adding Data Layout.")
        for i, node in enumerate(self.DORY_Graph):
            for name in node.constant_names:
                if name not in ["l","k","outshift","outmul", "inmul1", "inmul2"]:
                    if "bias" not in name:
                        weights_name = name
            if weights_name in node.__dict__:
                if node.name in "FullyConnected":
                    if node.__dict__[weights_name]["value"].shape[0] == node.input_channels:
                        node.__dict__[weights_name]["layout"] = "CinCout"
                    else:
                        node.__dict__[weights_name]["layout"] = "CoutCin"
                else:
                    node.__dict__[weights_name]["layout"] = "CoutCinK"
            node.layout = "CHW"



