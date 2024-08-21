import torch
from torch import nn
from torch.utils.data import DataLoader
from torchvision import datasets
from torchvision.transforms import transforms
import riscv_characterization.vendor.nemo as nemo
import numpy as np
from pathlib import Path
import json
import onnx
from packaging import version
from neural_networks.MNIST.mnist_net import MNIST_model1, precision_std
import nemo_training_quantization.utils as nemo_utils

# model pth to be loaded and quantized
model_name = "model1"
# onnx and json file names to be saved
#onnx_id_name = "./nemo_training_quantization/onnx/new_wd_model.onnx"
act_json = "./nemo_training_quantization/act.json"
# act_json will contains the act rescaling values obtained by NEMO.
# These are saved to load directly the id nemo model in future stages 
ex_dir = "./nemo_training_quantization/out_exact/"
Path(ex_dir).mkdir(parents=True, exist_ok=True)
#Path('./nemo_training_quantization/onnx').mkdir(parents=True, exist_ok=True)

model = MNIST_model1()
transform = transforms.Compose([transforms.ToTensor(), transforms.Normalize(0.1306, 0.308),transforms.Normalize(-0.5,1)])
bs = 32
test_data = datasets.MNIST(
    root="data",
    train=False,
    download=True,   # set to false if the dataset has been downloaded already
    transform=transform,
)

test_dataloader = DataLoader(test_data, batch_size=bs)
device = "cuda" if torch.cuda.is_available() else "cpu"
print("Using {} device".format(device))
model.load_state_dict(torch.load("./nemo_training_quantization/saved_models/" + model_name + '.pth')['model_state_dict'], strict=True)

model.to(device)
model_parameters = filter(lambda p: p.requires_grad, model.parameters())
print(model)
loss_fn = nn.NLLLoss()  # result is the same using this or CrossEntropy if softmax is in the model


_ = nemo_utils.test(test_dataloader, model, loss_fn, device, False )
nemo_utils.print_acc(test_data, test_dataloader, model, device, False)


print ("Go to NEMO fake quantized model")
dummy_input = torch.randn((1,1,28,28)).to(device)
nemo_model = nemo.transform.quantize_pact(model, dummy_input=dummy_input)
print(nemo_model)

nemo_model.change_precision(bits=1, min_prec_dict=precision_std)
_ = nemo_utils.test(test_dataloader, nemo_model, loss_fn, device, False)
nemo_utils.print_acc(test_data, test_dataloader, nemo_model, device, False)

print(nemo_model)

# adjust alpha value, according to statistics
with nemo_model.statistics_act():
    _ = nemo_utils.test(test_dataloader, nemo_model, loss_fn, device, False)
print("Reset alpha act")
nemo_model.reset_alpha_act()
_ = nemo_utils.test(test_dataloader, nemo_model, loss_fn, device, False)
nemo_utils.print_acc(test_data, test_dataloader, nemo_model, device, False)

print(nemo_model)

## folding batch norm
nemo_model.fold_bn()
nemo_model.reset_alpha_weights()
nemo_model = nemo.transform.bn_to_identity(nemo_model) # necessary because folding does not physically remove BN layers

print ("Performing quantized deployable transformations")
nemo_model.qd_stage(eps_in=1./255)
_ = nemo_utils.test(test_dataloader, nemo_model, loss_fn, device, False)
nemo_utils.print_acc(test_data, test_dataloader, nemo_model, device, False)


print ("Performing integer deployable transformations")
nemo_model.id_stage()
_ = nemo_utils.test(test_dataloader, nemo_model, loss_fn, device, integer=True)
nemo_utils.print_acc(test_data, test_dataloader, nemo_model, device, integer = True)

# save model state dict
batch_size = bs
epochs = 30
lr = 3e-3
wd=1e-3
Path("./nemo_training_quantization/saved_models_nemo").mkdir(parents=True, exist_ok=True)

PATH = "./nemo_training_quantization/saved_models_nemo/nemo_mnist_model.pth"
torch.save({ 'model_state_dict': nemo_model.state_dict(),
            'device': device,
            'train_parameters': {'batch': batch_size, 'epochs': epochs, 'lr': lr, 'wd': wd}
        }, PATH)

# save info on PACT activations to be able to completely reload the model
nemo_utils.write_act_parameters(nemo_model, act_json)

#write file with all intermediate results
in_t = (torch.randint(low=0, high=256, size=(1,1,28,28), dtype=torch.float32)).to(device)
nemo_utils.write_intermidiate_results(nemo_model, in_t, ex_dir)
nemo_utils.write_weights(nemo_model,ex_dir)


# Save and modify onnx model
#nemo.utils.export_onnx(onnx_id_name, nemo_model, nemo_model, (1,28,28), device)
#onnx_model = onnx.load(onnx_id_name)
#nemo_utils.rename_edges_onnx(onnx_model, onnx_id_name)


