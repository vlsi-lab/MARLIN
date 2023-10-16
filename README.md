![](https://raw.githubusercontent.com/vlsi-lab/MARLIN/main/marlin_logo.png)
# About
This repository contains the code used in the research paper "**MARLIN: a Co-design Methodology for Approximate ReconfigurabLe Inference of Neural Networks at the Edge**".
The code can be used as it is to reproduce the results presented in the paper.

MARLIN is a methodology that enables the implementation of approximate neural networks (NNs) on a RISC-V core.
It is built from 4 components:
- A runtime reconfigurable multiplier with 256 approximation levels, each one with its own error distribution and average power.
- A modified RISC-V core, with the multiplier inserted in the execution stage and the  
- A modified PULP toolchain, with automatic emb
- A search algorithm based on NSGA-II that evaluate thousands of approxximate NNs and select those with the best energy/accuracy trade-off.

## Install instructions
Execute the following commands to install 
Clone the repository with 
```bash
git clone https://github.com/vlsi-lab/MARLIN.git
```
or 
<<<<<<< HEAD

> git clone git@github.com:vlsi-lab/MARLIN.git

Then open a terminal and install ninja-build
> sudo apt update
>
> sudo apt install ninja-build
=======
```bash
git clone git@github.com:vlsi-lab/MARLIN.git
```
Then open a terminal and intall ninja-build
```bash
sudo apt update
>>>>>>> ed4bcd6d57ccc72ea9af765e7147a49769bf775d

sudo apt install ninja-build
```
Install marlin environment (requires anaconda or miniconda https://www.anaconda.com/download)
```bash
cd MARLIN

conda env create -f marlin.yml
```
Inside each folder there are README files with additional information useful to install additional components, execute the code and replicate the data. 

In order to use the multipliers LUTs and pre-trained neural networks, it is necessary to install 7-zip and extract the files according to the README contained in each folder. You can download 7-zip at https://www.7-zip.org/download.html
When launching any python file, remember to keep the folder hierarchy as defined in this repository.
You can add the folders to the PYTHONPATH with the following command:
```bash
export PYTHONPATH="${PYTHONPATH}:your_path_to_marlin/MARLIN/:your_path_to_marlin/MARLIN/neural_networks:your_path_to_marlin/MARLIN/benchmark_CIFAR10:your_path_to_marlin/MARLIN/approximate_multiplier$:your_path_to_marlin/MARLIN/riscv_characterization"$
```

Support to verify the experimental results obtained on the modified version of PULP Platform is provided. 
Execute the command:
```bash
make build
```
This command will setup the required scripts and repository. Further information on the configuration can be found in the folder `./riscv_characterization`.

## Citation
If you used this code in your work, consider adding a reference to our paper **editorial information to be added after publishing**