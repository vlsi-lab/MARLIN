# `dory_input_generation`

## Table of contents
- [`dory_input_generation`](#dory_input_generation)
  - [Table of contents](#table-of-contents)
  - [Folder organization](#folder-organization)
  - [Setup](#setup)
  - [Dependencies](#dependencies)


## Folder organization
This folder contains the Python scripts required to generate all info to be provided to DORY as inputs.
These are:
- ONNX model of the NN.
- JSON dictionary of the NN, containing the specific configuration of the multiplier (i.e. approx and precision level) for each linear and conv layer. 
- One input feature map with all the corresponding outputs produced by each activation function in the form of a txt file. This is required by DORY to check the correspondence between the high-level PyTorch/NEMO model and the low-level C/RTL-simulated model.

These files are stored together in a folder that is created during the execution of `DORY_preprocessing_ax.py`.

The additional script `DORY_preprocessing_exact.py` is instead intended to generate DORY inputs in the case we want to use the exact RI5CY architecture or completely bypass the approximate unit. In this last scenario, the JSON dictionary is not required; DORY conventional info are enough.

## Setup
After a correct setup of the Python environment, you can run the two scripts using additional arguments.

```bash
python DORY_preprocessing_ax.py --number N --list_idx IDX --dory_folder ./MYPATH  
python DORY_preprocessing_exact.py --number N --dory_folder ./MYPATH 
``` 
- `--number N` : sets the image class N, that is a number between 0 and 9 in case of MNIST, to be selected as an input to the NN.
- `list_idx IDX` : `DORY_preprocessing_ax.py` creates a list of solutions starting from the Pareto front output of NSGA-II.  The element at position IDX in the list is selected.
- `dory_folder ./MYPATH` : provide the Path to the folder where to store the info to provide DORY afterwards. These results should be either saved or copied to the correct path expected by DORY (it is configurable through a JSON file. See https://github.com/pulp-platform/dory.git for further info).

``` bash
python DORY_preprocessing_ax.py --help
python DORY_preprocessing_exact.py --help
```
Run these commands to have info on the arguments supported by the two scripts.

## Dependencies
This step of the procedure requires:
- The original PyTorch model definition.
- The state dictionary of the quantized model that is a .pth file, which should be stored as `/nemo_training_quantization/saved_models_nemo/new_wd_model`.
- The calibration factors produced by NEMO, should be stored as `./nemo_training_quantization/act.json`
- Some functions defined in `riscv_characterization/nemo_training_quantization/utils.py`.

