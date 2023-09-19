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
> git clone https://github.com/vlsi-lab/MARLIN.git
or 
> git clone git@github.com:vlsi-lab/MARLIN.git

Then open a terminal and intall ninja-build
> sudo apt update
> sudo apt install ninja-build

Install marlin environment (requires anaconda or miniconda https://www.anaconda.com/download)
> cd MARLIN
> conda env create -f marlin.yml

Inside each folder there are README files with additional information useful to install additional components, execute the code and replicate the data. 