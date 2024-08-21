import torch, torchvision, copy
from torch import nn
from torch.utils.data import DataLoader
from torchvision import datasets
from torchvision.transforms import transforms
import numpy as np
from tqdm import tqdm
from pathlib import Path
import matplotlib.pyplot as plt
from neural_networks.MNIST.mnist_net import MNIST_model1

"""
Train MNIST_model1 and save pth model to PATH. 
To have more control on hyperparameters use `neural_networks/MNIST/train_mnist.py`
"""

# TODO: incorporate in mnist_training
if __name__ == '__main__':
    # folder where to save relevant info
    PATH = "./nemo_training_quantization/saved_models/model1.pth"
    
    # flag to load a pre-trained model
    load_pretrained = False 

    transform_train = transforms.Compose([transforms.ToTensor(), transforms.Normalize(0.1306, 0.308), transforms.Normalize(-0.5,1)])
    transform_test = transforms.Compose([transforms.ToTensor(), transforms.Normalize(0.1306, 0.308), transforms.Normalize(-0.5,1)])

    training_data = datasets.MNIST(root="data", train=True, download=True, transform=transform_train)
    test_data = datasets.MNIST(root="data", train=False, download=True, transform=transform_test)

    # training parameters

    batch_size = 32
    epochs = 30
    lr = 3e-3  # learning rate 
    wd = 1e-3  # weight decay
    best_workers = 8

    train_dataloader = DataLoader(training_data, batch_size=batch_size, shuffle=True, num_workers=best_workers,
                                  pin_memory=torch.cuda.is_available())
    test_dataloader = DataLoader(test_data, batch_size=batch_size, shuffle=True, num_workers=best_workers,
                                 pin_memory=torch.cuda.is_available())

    for X, y in test_dataloader:
        print("Shape of X [N, C, H, W]: ", X.shape)
        print("Shape of y: ", y.shape, y.dtype)
        break


    device = "cuda" if torch.cuda.is_available() else "cpu"
    print("Using {} device".format(device))

    # call model defined in torch__networks_library.py 
    model = MNIST_model1()  # create model instance, initialize parameters, send to device
    model_parameters = filter(lambda p: p.requires_grad, model.parameters())
    print(model)
    model.to(device)
    
    params = sum([np.prod(p.size()) for p in model_parameters])

    for name, par in model.named_parameters():
        print("Layer", name, "Size:", par.size())

    memory = params * 32 / 8 / 1024 / 1024
    print("this model has ", params, " parameters")
    print("total weight memory is %.4f MB" % (memory))

    loss_fn = nn.NLLLoss()
    optimizer = torch.optim.SGD(model.parameters(), lr=lr, momentum=0.9,
                                weight_decay=wd)  # simple SGD with momentum and weight decay
    # scheduler for the learning rate
    scheduler = torch.optim.lr_scheduler.StepLR(optimizer,step_size=5, gamma=0.3)

    start_epoch = 0

    if load_pretrained:
        checkpoint = torch.load(PATH, map_location=device)
        model.load_state_dict(checkpoint['model_state_dict'])
        optimizer.load_state_dict(checkpoint["optimizer_state_dict"])
        start_epoch = checkpoint['epoch'] + 1
        epochs += start_epoch
        last_loss = checkpoint['loss']
        last_test = checkpoint['test_acc']

        print("loading pre-trained model with %.4f train loss, %.4f test accuracy, trained for %d epochs" % (
        last_loss, last_test, start_epoch))

    model.to(device)


    def train(dataloader, model, loss_fn, optimizer, epoch):
        size = len(dataloader.dataset)
        model.train()

        for batch, (X, y) in enumerate(dataloader):
            X, y = X.to(device), y.to(device)

            # Compute prediction error
            pred = model(X)
            loss = loss_fn(pred, y)
            #loss = F.nll_loss(pred,y)

            # Backpropagation
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            if batch % 100 == 0:
                loss, current = loss.item(), batch * len(X)
                print(f"loss: {loss:>7f}  [{current:>5d}/{size:>5d}]")
        return loss


    def test(dataloader, model, loss_fn):
        size = len(dataloader.dataset)
        num_batches = len(dataloader)
        model.eval()
        test_loss, correct = 0, 0
        with torch.no_grad():
            for X, y in dataloader:
                X, y = X.to(device), y.to(device)
                pred = model(X)
                test_loss += loss_fn(pred, y).item()
                correct += (pred.argmax(1) == y).type(torch.float).sum().item()
        test_loss /= num_batches
        correct /= size
        print(f"Test Error: \n Accuracy: {(100 * correct):>0.1f}%, Avg loss: {test_loss:>8f} \n")
        return correct


    best_correct = 0
    best_model = []
    Path(PATH).mkdir(parents=True, exist_ok=True)
    print("Use $ tensorboard --logdir=runs to access training statistics")
    for t in tqdm(range(start_epoch, epochs)):
        print(f"Epoch {t + 1}\n-------------------------------")
        loss = train(train_dataloader, model, loss_fn, optimizer, t)
        scheduler.step()
        new_lr = scheduler.get_last_lr()
        print("lr=",new_lr)
        current_correct = test(test_dataloader, model, loss_fn)
        ep = t + 1

        # save dictionary and other info
        torch.save({
            'epoch': t,
            'model_state_dict': model.state_dict(),
            'optimizer_state_dict': optimizer.state_dict(),
            'loss': loss,
            'test_acc': current_correct,
            'device': device,
            'train_parameters': {'batch': batch_size, 'epochs': epochs, 'lr': lr, 'wd': wd}
        }, PATH)

    print("Saved PyTorch Model State to model.pth")
    
    classes = ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9')

    correct_pred = {classname: 0 for classname in classes}
    total_pred = {classname: 0 for classname in classes}

    with torch.no_grad():
        for X, y in test_dataloader:
            images, labels = X.to(device), y.to(device)
            outputs = model(images)
            # print(outputs)
            _, predictions = torch.max(outputs, 1)
            for label, prediction in zip(labels, predictions):
                if label == prediction:
                    correct_pred[classes[label]] += 1
                total_pred[classes[label]] += 1

    min_correct = [0, 110]
    for classname, correct_count in correct_pred.items():
        accuracy = 100 * float(correct_count) / total_pred[classname]
        if min_correct[1] >= int(accuracy):
            min_correct = [classname, accuracy]
        print("Accuracy for class {:5s} is: {:.1f} %".format(classname, accuracy))

    lowest_class_accuracy = min_correct[1]

    print("Worst class accuracy is %.4f for class %s" % (min_correct[1], min_correct[0]))

