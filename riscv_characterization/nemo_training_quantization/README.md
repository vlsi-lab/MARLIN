# `nemo_training_quantization`

## Table of contents
- [`nemo_training_quantization`](#nemo_training_quantization)
  - [Table of contents](#table-of-contents)
  - [Folder organization](#folder-organization)
  - [Setup](#setup)

## Folder organization
This folder is the main one used for training, quantization and inference with NEMO on MNIST dataset. **Forse no**
The folder is organized as follows:
- `MNIST_training.py`: is the script for the training of the model, in full precision with PyTorch classes. Saves the final model in pyton format with its state dictionary. This is the input for the next step, that is NEMO quantization. **Potrebbe non essere più qui**
- `NEMO_quantization.py`: the trained model is loaded and transformed through NEMO modified classes. After the final model is obtained, some info which are useful for the next steps (approximation and DORY):
    - .pth file storing the state dictionary of the id model.
    - `act.json`: stores the activation rescaling factors. Useful as in this way the id model returner by NEMO can be directly reloaded without undergoing all the quantization steps everytime.  
- `NEMO_node.py`: define functions to create NEMO nodes and a dictionary containing info on every node of the Axx-NN graph.
- `utils.py`:
**It supports also models featuring bias, with no contranst in NEMO and DORY.**

## Setup
This step of the procedure requires the installation of NEMO Python package and its modification according to the instructions provided in [`riscv_characterization/repo_changes/nemo-mod/`](https://github.com/vlsi-lab/MARLIN/tree/main/riscv_characterization/repo_changes/nemo-mod). 
NEMO can be installed in the conda environment using the command:
``` bash
pip install pytorch-nemo
```
or you can clone the repository following the instructions in [`riscv_characterization/repo_changes/`](https://github.com/vlsi-lab/MARLIN/tree/main/riscv_characterization/repo_changes).
For further information on the package setup we encourage you to follow the README.md in the original [nemo](https://github.com/pulp-platform/nemo) repository.
