# `dory_input_generation`
This folder contains the Python scripts required to generate all info to be provided to DORY as inputs.
These are:
- ONNX model of the NN.
- JSON dictionary of the NN, containing the specific configuration of the multiplier (i.e. approx and precision level) for each linear and conv layer. 
- One input feature map with all the corresponding outputs produced by each activation function in the form of a txt file. This is required by DORY to check the correspondence between the high-level Pythorch/NEMO model and the low-level C/RTL-simulated model.

The last two group of files are stored together in a folder that is created during the execution of `DORY_preprocessing_ax.py`.

The additional script `DORY_preprocessing_exact.py` is instead intended to generate DORY inputs in the case we want to use the exact RI5CY architecture or completely bypass the approximate unit. In this last scenario, the JSON dictionary is not required; DORY conventional info are enough.
 
