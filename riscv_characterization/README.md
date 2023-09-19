# `riscv_characterization`
This folder contains all the files required to validate the entire MARLIN flow, for a simple NN on MNIST dataset, on PULP platform.
The covered steps are here summarized:
1. Definition of a NN model suitable for the MNIST dataset. 
2. Full-precision floating-point training of the model selecting suitable hyperparameters.
3. Post-training quantization and calibration of the NN transforming its parameters into integers.
4. Layer-wise approximation of the NN using NSGA-II as an optimization strategy. 
5. Select one solution from NSGA-II run and port the Axx-NN into C executable code compatible with the approximate PULP Platform.


The folder is organized in several sub-folders which implement the code required to accomplish the different steps previously described.
Their main purpose is here summarized and is described in greater detail in the README file inside each folder.
- `nemo_training_quantization`: groups the scripts for training and quantization through NEMO library. 
- `Utils`: contains useful functions which can be shared by different steps. It also contains the definition of the NN model.
- `NSGA2_compile_ultra`: the NSGA-II optimization algorithm is in this folder.


All Python scripts are launched from this level of the repository, thus the relative paths are accordingly defined.
 
