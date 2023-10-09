# `riscv_characterization`

## Table of contents
- [`riscv_characterization`](#riscv_characterization)
  - [Table of contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Folder organization](#folder-organization)
  - [Setup](#setup)

## Introduction
This folder contains all the files required to validate the entire MARLIN flow for a simple NN on MNIST dataset, on PULP platform.
The covered steps are here summarized:
1. Post-training quantization and calibration of a pre-trained full-precision NN, transforming its parameters into integers.
2. Layer-wise approximation of the NN using NSGA-II as an optimization strategy. 
3. Select one solution from NSGA-II run and port the Axx-NN into C executable code compatible with the approximate PULP Platform.

## Folder organization
The folder is organized in several sub-folders which implement the code required to accomplish the different steps previously described.
Their main purposes are here summarized and described in greater detail in the README file inside each folder.
- `nemo_training_quantization`: groups the scripts for training and quantization through NEMO library.
- `NSGA2`: the NSGA-II optimization algorithm is in this folder.
- `dory_inputs_generation`: folder grouping scripts to generate input files required by DORY.
- `repo_changes`: this folder groups all the files that have been modified into PULP repositories. It gives information on dependencies and versions. 
- `vendor`: this folder contains the configuration files used by the vendor tool to keep track of the modified repositories. Vendorized repo will be cloned into this folder. 
- `dory_model.7z`: contains the output ONNX, JSON and the text files produced by out model after running the script `riscv_characterization/dory_inputs_generation/DORY_preprocessing_ax.py`. These files are provided to give an example of the inputs our approximate version of DORY expects. 

## Setup
PULP repositories with all the modifications performed to support our approximate multiplier are managed through [vendor](https://opentitan.org/book/util/doc/vendor.html#utilvendorpy-vendor-in-components) tool.
All the repo related to the PULP project are automatically cloned, together with sub-modules, into the `vendor` folder when the make command is launched. **To be implemented** Further details on the setup of the specific tools and simulations are provided in the target sub-directories.


 
