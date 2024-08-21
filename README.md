![](https://raw.githubusercontent.com/vlsi-lab/MARLIN/main/marlin_logo.png)
# About
This repository contains the code used in the research paper "**MARLIN: a Co-design Methodology for Approximate ReconfigurabLe Inference of Neural Networks at the Edge**", published in "IEEE Transactions on Circuits and Systems I: Regular Papers".
The code can be used as it is to reproduce the results presented in the paper.

MARLIN is a methodology that enables the implementation of approximate neural networks (NNs) on a RISC-V core.
It is built from 4 components:
- A runtime reconfigurable multiplier with 256 approximation levels, each one with its own error distribution and average power.
- A modified RISC-V core, with the multiplier inserted in the execution stage. 
- A modified PULP toolchain, with approximation control automatically embedded in the compiled code.
- A search algorithm based on NSGA-II that evaluates thousands of approxximate NNs and selects those with the best energy/accuracy trade-off.

## Install instructions
Execute the following commands to install 
Clone the repository with 
```bash
git clone https://github.com/vlsi-lab/MARLIN.git
```
or
```bash
git clone git@github.com:vlsi-lab/MARLIN.git
```
Then open a terminal and intall ninja-build
```bash
sudo apt update
sudo apt install ninja-build
```
Install marlin environment (requires miniconda or [anaconda](https://www.anaconda.com/download)).
```bash
cd MARLIN

conda env create -f marlin.yml
```
Inside each folder there are README files with additional information useful to install additional components, execute the code and replicate the data. 

In order to use the multiplier LUTs and pre-trained neural networks, it is necessary to install 7-zip and extract the files according to the README contained in each folder. You can download 7-zip from [here](https://www.7-zip.org/download.html).
When launching any python file, remember to keep the folder hierarchy as defined in this repository.
You can add the folders to the PYTHONPATH, after entering the MARLIN repo, with the following command:
```bash
export PYTHONPATH="${PYTHONPATH}:./:./neural_networks:./benchmark_CIFAR10:./approximate_multiplier:./riscv_characterization"
```

## Run simulation
Support to verify the experimental results obtained on the modified version of PULP Platform is provided.
[vendor](https://opentitan.org/book/util/doc/vendor.html#utilvendorpy-vendor-in-components) is used to keep track of dependencies across multiple github repositories.
Execute the command:
```bash
make build
```
This command will setup the required scripts and repository. Further information on the configuration can be found in the folder `./riscv_characterization`.\
*Note:* to avoid possible path errors, we highly suggest to launch python scripts from inside `MARLIN` folder directly.

## Citation
If you used this code in your work, consider adding a reference to our paper 
```
@ARTICLE{marlin,
  author={Guella, Flavia and Valpreda, Emanuele and Caon, Michele and Masera, Guido and Martina, Maurizio},
  journal={IEEE Transactions on Circuits and Systems I: Regular Papers}, 
  title={MARLIN: A Co-Design Methodology for Approximate ReconfigurabLe Inference of Neural Networks at the Edge}, 
  year={2024},
  volume={71},
  number={5},
  pages={2105-2118},
  doi={10.1109/TCSI.2024.3365952}}
```
