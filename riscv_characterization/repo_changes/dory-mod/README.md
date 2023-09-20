# `dory-mod`
The original repository version is [dory](https://github.com/pulp-platform/dory.git)
commit 0e9011e752476c66c12770cfd283f826bdd5428a

To correctly clone the submodules first you need to delete the empty folders, then execute:
```
git submodule update --remote --init dory/Hardware_targets/PULP/Backend_Kernels/pulp-nn
git submodule update --remote --init dory/Hardware_targets/PULP/Backend_Kernels/pulp-nn-mixed
```
For `pulp-nn` commit 9ada4a93bd9704bda0da29be1029ffe8f1362428
For `pulp-nn-mixed` commit 46cbceb9dfd8bd2febdcd30c443fe9bbb35f0a26 (not actually employed in this work
**Careful in case the submodules could be at a wrong version**
In addition to those files, a further copy of the directory `pulp-nn`, exactly as it is but renamed to `pulp-nn_exact` should be stored in its same path.
This directory is the one storing the library functions to be called in case the exact architecture is chosen, while `pulp-nn` will be modified to be used in case of mixed-precision, approximate unit activation.
 
## Paths
- `network_generate.py`: `dory/`
- `Parser.py`: `dory/Frontend_frameworks/NEMO/`
- `Pattern_rewriter.py`: `dory/Frontend_frameworks/NEMO/` 
- `DORY_node.py`: `dory/Parsers/`
- `Layer_node.py`: `dory/Parsers/`
- `Parser_ONNX_to_DORY.py`: `dory/Parsers/`
- `Layer2D_template_writer.py`: `dory/Utils/Templates_writer/`
- `C_Parser.py`: `dory/Hardware_targets/PULP/Common`
- `HW_description.json`: `dory/Hardware_targets/PULP/PULP_gvsoc/`
- `layer_L2_c_conv_template.c`: `dory/Hardware_targets/PULP/PULP_gvsoc/Templates/layer_templates/`
- `pulp_nn_conv_Ho_parallel.c`: `dory/Hardware_targets/PULP/Backend_Kernels/pulp-nn/32bit/src/`
- `pulp_nn_linear.c`: `dory/Hardware_targets/PULP/Backend_Kernels/pulp-nn/32bit/src/`
- `pulp_nn_linear_out_32.c`: `dory/Hardware_targets/PULP/Backend_Kernels/pulp-nn/32bit/src/`

**Notice, there could be another `dory/` folder in the hierarchy which has to be added**

## Changes
-`network_generate.py`: added `--arch` option to select either the exact architecture (cluster made of exact RI5CY cores) or the approximate one.
- `Parser.py`: modify the function to assign precision. This implementation allows to use mixed precision (out of the immediate scope of this work). Add to the constructor a configuration file, that is the json file produced by the script in `dory_input_generation` folder, used to store approximation and precision info for Conv and linear nodes.
- `Pattern_rewriter.py`: read for every node type parsed from the NEMO graph precision and approximation info.
- `DORY_node.py`: for each DORY node add functions to read from the corresponding node in the NEMO graph precision and approximation parameters. 
- `Layer_node.py`: add functions for precision and approximation selection of nodes.
- `Parser_ONNX_to_DORY.py`: call the additional function for approximation on DORY and layer nodes. 
- `Layer2D_template_writer.py`: added parameters required for correct CSR configuration and modified some others to account for the mixed precision case. 
- `C_Parser.py`: adding distinction between execution with exact and approx architecture. 
- `HW_description.json`: change name of the software development kit from gap-sdk to pulp-sdk. 
- `layer_L2_c_conv_template.c`: adding in the correct position the CSR instruction configuring the approximate unit, but leaving it inactive.
- `pulp_nn_conv_Ho_parallel.c`: add CSR instructions to activate and deactivate approx unit, accordingly to the disassembly produced by the tested model. 
- `pulp_nn_linear.c`: add CSR instructions to activate and deactivate approx unit, accordingly to the disassembly produced by the tested model. 
- `pulp_nn_linear_out_32.c`: add CSR instructions to activate and deactivate approx unit, accordingly to the disassembly produced by the tested model. 

Pulp-nn library files may require additional modifications in case of more complex NN models. In particular, the points where the CSR instructions should be inserted highly depends on parameters evaluated by DORY and could be optimized depending on the specific NN processed.

## Input Files
DORY requires some input files describing the NN model in order to produce the relative executable C code tailored on a specific HW platform (PULP in our case).
Standard DORY requires:
- ONNX graph of the quantized NN obtained with NEMO (or QuantLAB).
- One example of input and the value, given that input, of all the outputs of each activation layer.
- A JSON file storing the path to the ONNX graph and other configuration info, at `dory/dory_examples/config_files/`.

In addition to these files, our modified version for AxxNN requires:
- a JSON file describing the graph (with same nodes names as in the ONNX) containing info on the precision and approximation each node features.

The standard JSON for configuration presents an additional field in its dictionary, called `nemo_json`, storing the path to the JSON graph. The input and output txt files have to be in the same folder of the ONNX.

All these files, except for the configuration JSON, are produced by the script in the folder `/dory_inputs_generation/`


**Capire se si può aggiungere anche lo script per runnare automaticamente che ho**
