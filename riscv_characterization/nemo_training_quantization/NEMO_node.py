import onnx
import numpy as np

class NEMO_node:
    """
    This is the definition of a class with its method  contaning information which will be then 
    used by Dory to support mixed precision and approximate operators

    """
    def __init__(self):
        self.name = None
        self.op_type = None
        self.constant_bits = None
        self.weight_bits = None
        self.bias_bits = None
        self.output_activation_bits = None
        self.input_activation_bits = None
        self.second_input_activation_bits = None
        self.weight_type = None
        self.constant_type = None
        self.output_activation_type = None
        self.input_activation_type = None
        self.second_input_activation_type = None
        self.approx_mul = None      # 1 if node uses approx mult
        self.approx_mac = None
        self.approx_dot8 = None
        self.precision_mul = None
        #self.approx_add = None      # 1 if node uses approx add
        self.mul_conf = None        # conf of the multiplier, approx level
        #self.add_conf = None        # conf of the adder, approx level


    def add_existing_parameter(self, key, value):
        if key in self.__dict__:
            self.__dict__[key] = value
        else:
            sys.exit("Adding {} parameter to a graph node. This parameter does not exist. ".format(key))

    def add_existing_dict_parameter(self, dict_parameters):
        for key, value in dict_parameters.items():
            self.add_existing_parameter(key, value)

    def get_parameter(self, name):
        return self.__dict__[name]
    
    def export_to_dict(self):
        node_dict = {}
        node_dict["name"] = self.name
        node_dict["DORY_node_parameters"] = {}
        for key, value in self.__dict__.items():
            if not isinstance(value, dict) and key != "name":
                node_dict["DORY_node_parameters"][key] = value
        return node_dict

    def check_uninitialized_parameters(self):
        for key, value in self.__dict__.items():
            if isinstance(value, type(None)) or (isinstance(value, list) and len(value) == 0):
                sys.exit("NEMO error. Missing some Node initialization. Stopping at argument {}".format(key))

    def populate_NEMO_node (self, node_iterating): 
        # starting from node_iterating, that is node in onnx file, define new graph node
        NEMO_parameters = {}
        NEMO_parameters['name'] = node_iterating.name #leave same name of onnx graph
        NEMO_parameters['op_type'] = node_iterating.op_type

        self.add_existing_dict_parameter(NEMO_parameters)

    def add_node_precision(self, node_iterating, precision_dict, model, NEMO_graph):
        """
        give as input the precision dictionary used by NEMO for its quantization
        find a correspondence between onnx nodes and nodes names in the model. In onnx>=1.12.0
        the name of the nodes contains the names in the pytorch model. This was not true in 
        version 1.8.1 where this method won't work
        """
        if (onnx.__version__ < '1.12.0' ) : 
            print("This procedure won't work unless you modify the onnx graph names as in your pytorch model")
        #else: ?
        prev_act_bits = 8 #initialization
        for n, m in model._modules.items():
            if(node_iterating.name.find(n)!= -1 ):
                if(node_iterating.op_type == 'Conv'):
                    self.add_existing_parameter("weight_bits",precision_dict[n]["W_bits"])
                    self.add_existing_parameter("output_activation_bits", 32)
                    self.add_existing_parameter("bias_bits", 32)
                    #retrieve from NEMO_graph the precision of the previous layer
                    try:
                        self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].output_activation_bits) #last element if in order nodes
                        precision_mul = precision_dict[n]["W_bits"] + NEMO_graph[-1].output_activation_bits +3 -4
                        self.add_existing_parameter("precision_mul", precision_mul)
                    except:
                        self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                        precision_mul = precision_dict[n]["W_bits"] + 8+3-4
                        self.add_existing_parameter("precision_mul", precision_mul)

                elif (m.__class__.__name__ == 'PACT_IntegerAct'): #part of the Act module
                    if(node_iterating.op_type in ['Clip', 'Constant', 'Cast']): #relu nodes qualcosa di simile per prendere tutti i nodi
                        self.add_existing_parameter("output_activation_bits", precision_dict[n]["x_bits"])
                        self.add_existing_parameter("input_activation_bits", 32) #fixed to 32, will then depend on previous Conv layer
                    
                    elif (node_iterating.op_type in ['Mul', 'Add', 'Div' ]):
                        #are part of some layer, could be ReLu in our case, or maybe others we haven't seen yet
                        self.add_existing_parameter("constant_bits", 32) #can be set to other values??
                        self.add_existing_parameter("output_activation_bits", precision_dict[n]["x_bits"])
                        self.add_existing_parameter("input_activation_bits", 32) #fixed to 32, will then depend on previous Conv layer
                        #for now we add to all nodes of Act the same info, as we do not know which one is actually present
                        #in general
                elif(node_iterating.op_type == 'MaxPool'): #maxpool
                    #save same precision at the output of the input 
                    try:
                        self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].output_activation_bits) #last element if in order nodes
                        self.add_existing_parameter("output_activation_bits",NEMO_graph[-1].output_activation_bits) #last element if in order nodes
                    except:
                        self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                        self.add_existing_parameter("output_activation_bits", 8) #last element if in order nodes
                
                elif(node_iterating.op_type == 'MatMul'):
                    self.add_existing_parameter("weight_bits",precision_dict[n]["W_bits"])
                    #use precision of the output of previous node 
                    try:
                        if (NEMO_graph[-1].op_type == "Flatten"):
                            try:
                                self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].input_activation_bits) #take input value, as flatten will not be considered
                                precision_mul = precision_dict[n]["W_bits"] + NEMO_graph[-1].input_activation_bits +3 -4
                                self.add_existing_parameter("precision_mul", precision_mul)
                            except:
                                self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                                precision_mul = precision_dict[n]["W_bits"] + 8 +3 -4
                                self.add_existing_parameter("precision_mul", precision_mul)
                        else :
                            self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].output_activation_bits) #last element if in order nodes
                            precision_mul = precision_dict[n]["W_bits"] + NEMO_graph[-1].output_activation_bits +2 -4
                            self.add_existing_parameter("precision_mul", precision_mul)
                    except:
                        self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                    self.add_existing_parameter("output_activation_bits", 32)
                    self.add_existing_parameter("constant_bits", 32) #can be set to other values??
                
                else: #cases not yet encountered,
                    try:
                        if (NEMO_graph[-1].op_type == "Flatten"):
                            try:
                                self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].input_activation_bits) #last element if in order nodes
                            except:
                                self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                        else :
                            self.add_existing_parameter("input_activation_bits",NEMO_graph[-1].output_activation_bits) #last element if in order nodes
                    except:
                        self.add_existing_parameter("input_activation_bits", 8) #last element if in order nodes
                    self.add_existing_parameter("output_activation_bits", 32)
                    self.add_existing_parameter("constant_bits", 32) #can be set to other values??



    def add_node_type(self):
        """
        can leave it for now and use Dory types
        """


    def add_node_approximation(self, node_iterating, approximation_dict, model):
        """assume our algorithm builds a dictionary similar to precision, where each node is identified
        by its name and then has a series of attributes: the ones we read here
        """
        if (onnx.__version__ < '1.12.0' ) : 
            print("This procedure won't work unless you modify the onnx graph names as in your pytorch model")
        #else: ?
        for n, m in model._modules.items():
            NEMO_parameters = {}
            if(node_iterating.name.find(n)!= -1 ):
                if(node_iterating.op_type == 'Conv'):
                    NEMO_parameters['approx_mul'] = approximation_dict[n]['approx_mul']
                    NEMO_parameters['approx_mac'] = approximation_dict[n]['approx_mac']
                    NEMO_parameters['approx_dot8'] = approximation_dict[n]['approx_dot8']
                    NEMO_parameters['mul_conf'] = approximation_dict[n]['mul_conf']
                     
                elif (m.__class__.__name__ == 'PACT_IntegerAct'): #part of the Act module
                    if(node_iterating.op_type == 'Clip'): #relu nodes qualcosa di simile per prendere tutti i nodi
                        NEMO_parameters['approx_mul'] = approximation_dict[n]['approx_mul']
                        NEMO_parameters['approx_mac'] = approximation_dict[n]['approx_mac']
                        NEMO_parameters['approx_dot8'] = approximation_dict[n]['approx_dot8']
                        NEMO_parameters['mul_conf'] = approximation_dict[n]['mul_conf']
                    
                    elif (node_iterating.op_type in ['Mul', 'Add', 'Div' ]):
                        #are part of some layer, could be ReLu in our case, or maybe others we haven't seen yet
                        NEMO_parameters['approx_mul'] = approximation_dict[n]['approx_mul']
                        NEMO_parameters['approx_mac'] = approximation_dict[n]['approx_mac']
                        NEMO_parameters['approx_dot8'] = approximation_dict[n]['approx_dot8']
                        NEMO_parameters['mul_conf'] = approximation_dict[n]['mul_conf']
                    
                elif(node_iterating.op_type == 'MaxPool'): #maxpool
                    #save same precision at the output of the input 
                    NEMO_parameters['approx_mul'] = approximation_dict[n]['approx_mul']
                    NEMO_parameters['approx_mac'] = approximation_dict[n]['approx_mac']
                    NEMO_parameters['approx_dot8'] = approximation_dict[n]['approx_dot8']
                    NEMO_parameters['mul_conf'] = approximation_dict[n]['mul_conf']
                    
                elif(node_iterating.op_type == 'MatMul'):
                    NEMO_parameters['approx_mul'] = approximation_dict[n]['approx_mul']
                    NEMO_parameters['approx_mac'] = approximation_dict[n]['approx_mac']
                    NEMO_parameters['approx_dot8'] = approximation_dict[n]['approx_dot8']
                    NEMO_parameters['mul_conf'] = approximation_dict[n]['mul_conf']
                    
                else: #case not yet encountered, batch, avgpool, globalavg, residual ecc 
                    NEMO_parameters['approx_mul'] = 0
                    NEMO_parameters['approx_mac'] = 0
                    NEMO_parameters['approx_dot8'] = 0
                    NEMO_parameters['mul_conf'] = 0
                    
                self.add_existing_dict_parameter(NEMO_parameters)
    
