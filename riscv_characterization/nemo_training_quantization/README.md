# `nemo_training_quantization`
This folder is the main folder used for training, quantization and inference with NEMO on MNIST dataset. 
The folder is organized as follows:
- `MNIST_training.py`: is the script for the training of the model, in full precision with PyTorch classes. Saves the final model in pyton format with its state dictionary. This is the input for the next step, that is NEMO quantization.
- `NEMO_quantization.py`: the trained model is loaded and transformed through NEMO modified classes. After the final model is obtained, some info which are useful for the next steps (approximation and DORY):
    - .pth file storing the state dictionary of the id model.
    - `act.json`: stores the activation rescaling factors. Useful as in this way the id model returner by NEMO can be directly reloaded without undergoing all the quantization steps everytime.  
  
**It supports also models featuring bias, with no contranst in NEMO and DORY. Questo valutare se va inserito o no, perchè nell'articolo non ne parliamo**
**Stabilire se vanno inclusi anche modelli.pth e act.json e onnx**
