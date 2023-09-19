import argparse
import copy
import time

import numpy as np
import torch
import torch.nn as nn
from pathlib import Path
from neural_networks.CIFAR10.resnet import resnet8, resnet14, resnet20, resnet32, resnet50, resnet56
from neural_networks.utils import (get_loaders_split, evaluate_test_accuracy)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--batch-size', default=100, type=int, help="Number of images processed during each iteration")
    parser.add_argument('--data-dir', default="/data/dataset/pytorch_only/", type=str, help="Directory in which the MNIST and FASHIONMNIST dataset are stored or should be downloaded")
    parser.add_argument('--epochs', default=128, type=int, help="Number of training epochs")
    parser.add_argument('--lr-max', default=1e-1, type=float, help="Maximum learning rate for 'cyclic' scheduler, standard learning rate for 'flat' scheduler")
    parser.add_argument('--lr-min', default=1e-4, type=float, help="Minimum learning rate for 'cyclic' scheduler")
    parser.add_argument('--lr-type', default="cyclic", type=str, help="Select learning rate scheduler, choose between 'cyclic' or 'flat'")
    parser.add_argument('--weight-decay', default=5e-4, type=float, help="Weight decay applied during the optimization step")
    parser.add_argument('--fname', default="baseline_model.pth", type=str, help="Name of the model, must include .pth")
    parser.add_argument('--num-workers', default=4, type=int, help="Number of threads used during the preprocessing of the dataset")
    parser.add_argument('--threads', default=16, type=int, help="Number of threads used during the inference, used only when neural-network-type is set to adapt")
    parser.add_argument('--seed', default=42, type=int, help="Seed for reproducible random initialization")
    parser.add_argument('--lr-momentum', default=0.9, type=float, help="Learning rate momentum")
    parser.add_argument('--split-val', default=0.1, type=float, help="The split-val is used to divide the training set in training and validation with the following dimensions: train=train_images*(1-split_val)  valid=train_images*split_val")
    parser.add_argument('--neural-network', default="resnet56", type=str, help="Choose one from resnet8, resnet14, resnet20, resnet32, resnet50, resnet56")
    parser.add_argument('--execution-type', default='float', type=str, help="Select type of neural network and precision. Options are: float, quant, adapt, fused, fused_test. \n float: the neural network is executed with floating point precision.\n quant: the neural network weight, bias and activations are quantized to 8 bit\n adapt: the neural network is quantized to 8 bit and processed with exact/approximate multipliers \n fused:the neural network is trained and executed with every normalization layer folded into the preceeding convolutional layer")
    parser.add_argument('--disable-aug', default=False, type=bool, help="Set to True to disable data augmentation to obtain deterministic results")
    parser.add_argument('--reload', default=True, type=bool, help="Set to True to reload a pretraind model, set to False to train a new one")

    return parser.parse_args()


def main():
    args = get_args()
    model_dir = "./neural_networks/models/"
    Path(model_dir).mkdir(parents=True, exist_ok=True)

    if args.execution_type == 'adapt':
        device = "cpu"
        torch.set_num_threads(args.threads)
    else:
        device = "cuda" if torch.cuda.is_available() else "cpu"

    if args.execution_type == 'float':
        filename = model_dir + args.neural_network + "_baseline_model.pth"
    elif args.execution_type == 'fused_test':
        filename = model_dir + args.neural_network + '_fused' + "_baseline_model.pth"
    elif args.execution_type == 'adapt':
        filename = model_dir + args.neural_network + '_quant_baseline_model.pth'
    else:
        filename = model_dir + args.neural_network + '_' + args.execution_type + "_baseline_model.pth"

    print(args.neural_network)
    print(args.execution_type)

    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.cuda.manual_seed(args.seed)

    train_loader, valid_loader, test_loader = get_loaders_split(args.data_dir, batch_size=args.batch_size,
                                                                dataset_type='cifar10',
                                                                num_workers=args.num_workers, split_val=args.split_val,
                                                                disable_aug=args.disable_aug)
    if args.neural_network == "resnet8":
        model = resnet8(args.execution_type).to(device)
    elif args.neural_network == "resnet20":
        model = resnet20(args.execution_type).to(device)
    elif args.neural_network == "resnet14":
        model = resnet14(args.execution_type).to(device)
    elif args.neural_network == "resnet32":
        model = resnet32(args.execution_type).to(device)
    elif args.neural_network == "resnet50":
        model = resnet50(args.execution_type).to(device)
    elif args.neural_network == "resnet56":
        model = resnet56(args.execution_type).to(device)
    else:
        exit("error unknown CNN model name")

    if args.reload:
        checkpoint = torch.load(filename, map_location=device)
        model.load_state_dict(checkpoint['model_state_dict'], strict=False)
        model.to(device)
        model.eval()
        test_loss, test_acc = evaluate_test_accuracy(test_loader, model, device)
        print(f'final test loss:{test_loss}, final test acc:{test_acc}')
        exit()

    model.train()

    opt = torch.optim.SGD(model.parameters(), lr=args.lr_max, momentum=args.lr_momentum, weight_decay=args.weight_decay)
    criterion = nn.CrossEntropyLoss()

    lr_steps = args.epochs * len(train_loader)
    if args.lr_type == 'cyclic':
        scheduler = torch.optim.lr_scheduler.CyclicLR(opt, base_lr=args.lr_min, max_lr=args.lr_max,
                                                      step_size_up=lr_steps / 2, step_size_down=lr_steps / 2)
    elif args.lr_type == 'multistep':
        scheduler = torch.optim.lr_scheduler.MultiStepLR(opt, milestones=[lr_steps / 2, lr_steps * 3 / 4], gamma=0.1)

    best_test_acc = 0
    # Training
    for epoch in range(args.epochs):
        start_epoch_time = time.time()
        train_loss = 0
        train_acc = 0
        train_n = 0
        for i, (X, y) in enumerate(train_loader):
            X, y = X.to(device), y.to(device)
            output = model(X)
            loss = criterion(output, y)
            opt.zero_grad()
            loss.backward()
            opt.step()
            scheduler.step()
            train_loss += loss.item() * y.size(0)
            train_acc += (output.max(1)[1] == y).sum().item()
            train_n += y.size(0)
        epoch_time = time.time()
        lr = scheduler.get_lr()[0]
        model.eval()
        test_loss, test_acc = evaluate_test_accuracy(valid_loader, model, device)
        if test_acc > best_test_acc:
            best_test_acc = test_acc
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'train_loss': train_loss / train_n,
                'train_acc': train_acc / train_n,
                'test_loss': test_loss,
                'test_acc': test_acc,
                'device': device,
                'train_parameters': {'batch': args.batch_size, 'epochs': args.epochs, 'lr': lr,
                                     'wd': args.weight_decay}
            }, filename)
        model.train()



        print(
            f'epoch:{epoch}, time:{epoch_time - start_epoch_time}, lr:{lr}, train loss:{train_loss / train_n}, train acc:{train_acc / train_n}, valid loss:{test_loss}, valid acc:{test_acc}')



    # Evaluation
    if args.neural_network == "resnet8":
        model = resnet8(args.execution_type).to(device)
    elif args.neural_network == "resnet14":
        model = resnet14(args.execution_type).to(device)
    elif args.neural_network == "resnet20":
        model = resnet20(args.execution_type).to(device)
    elif args.neural_network == "resnet32":
        model = resnet32(args.execution_type).to(device)
    elif args.neural_network == "resnet50":
        model = resnet50(args.execution_type).to(device)
    elif args.neural_network == "resnet56":
        model = resnet56(args.execution_type).to(device)

    model.load_state_dict(torch.load(filename, map_location='cpu')['model_state_dict'])
    model.float()
    model.to(device)
    model.eval()
    test_loss, test_acc = evaluate_test_accuracy(test_loader, model, device)
    print(f'FINAL test loss:{test_loss}, test acc:{test_acc}')


if __name__ == "__main__":
    main()
