import argparse
import time
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
from pathlib import Path

from neural_networks.MNIST.mnist_net import quant_mnist_net, adapt_mnist_net
from neural_networks.utils import get_loaders_split, evaluate_test_accuracy


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--batch-size', default=100, type=int, help="Number of images processed during each iteration")
    parser.add_argument('--data-dir', default="/data/dataset/pytorch_only/", type=str, help="Directory in which the MNIST and FASHIONMNIST dataset are stored or should be downloaded")
    parser.add_argument('--epochs', default=16, type=int, help="Number of training epochs")
    parser.add_argument('--lr-max', default=5e-3, type=float, help="Maximum learning rate for 'cyclic' scheduler, standard learning rate for 'flat' scheduler")
    parser.add_argument('--lr-type', default="cyclic", type=str, help="Select learning rate scheduler, choose between 'cyclic' or 'flat'")
    parser.add_argument('--weight-decay', default=1e-4, type=float, help="Weight decay applied during the optimization step")
    parser.add_argument('--fname', default="baseline_model.pth", type=str, help="Name of the model, must include .pth")
    parser.add_argument('--num-workers', default=4, type=int, help="Number of threads used during the preprocessing of the dataset")
    parser.add_argument('--threads', default=8, type=int, help="Number of threads used during the inference, used only when neural-network-type is set to adapt")
    parser.add_argument('--seed', default=42, type=int, help="Seed for reproducible random initialization")
    parser.add_argument('--lr-momentum', default=0.9, type=float, help="Learning rate momentum")
    parser.add_argument('--neural-network-type', default='adapt', type=str, help="either quant or adapt")
    parser.add_argument('--dataset-type', default='fashionmnist', type=str, help="Can select either fashionmnist or mnist")
    parser.add_argument('--split-val', default=0.2, type=float, help="The split-val is used to divide the training set in training and validation with the following dimensions: train=train_images*(1-split_val)  valid=train_images*split_val")

    return parser.parse_args()


def main():
    args = get_args()
    if args.neural_network_type == 'adapt':
        device = "cpu"
        torch.set_num_threads(args.threads)
    else:
        device = "cuda" if torch.cuda.is_available() else "cpu"
    Path("./neural_networks/models").mkdir(parents=True, exist_ok=True)

    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.cuda.manual_seed(args.seed)

    train_loader, valid_loader, test_loader = get_loaders_split('/data/dataset/pytorch_only', batch_size=args.batch_size, dataset_type=args.dataset_type,num_workers=args.num_workers, split_val=args.split_val)

    model = quant_mnist_net() if args.neural_network_type == "quant" else adapt_mnist_net()
    model.to(device)
    model.train()

    opt = torch.optim.SGD(model.parameters(), lr=args.lr_max, momentum=args.lr_momentum, weight_decay=args.weight_decay, nesterov=True)

    if args.lr_type == 'cyclic':
        lr_schedule = lambda t: np.interp([t], [0, args.epochs * 2 // 5, args.epochs], [0, args.lr_max, 0])[0]
    elif args.lr_type == 'flat':
        lr_schedule = lambda t: args.lr_max
    else:
        raise ValueError('Unknown lr_type, only options are cyclic or flat')

    loss_fn = nn.CrossEntropyLoss()
    best_valid_acc = 0
    for epoch in range(args.epochs):

        start_time = time.time()
        train_loss = 0
        train_acc = 0
        train_n = 0
        for i, (X, y) in enumerate(train_loader):
            X, y = X.to(device), y.to(device)
            lr = lr_schedule(epoch + (i + 1) / len(train_loader))
            opt.param_groups[0].update(lr=lr)
            output = model(X)
            loss = loss_fn(output, y)
            opt.zero_grad()
            loss.backward()
            opt.step()
            train_loss += loss.item() * y.size(0)
            train_acc += (output.max(1)[1] == y).sum().item()
            train_n += y.size(0)

        train_time = time.time()
        model.eval()

        valid_loss, valid_acc = evaluate_test_accuracy(valid_loader, model, device)

        model.train()

        if best_valid_acc < valid_acc:
            best_valid_acc = valid_acc
            if args.neural_network_type == 'adapt':
                torch.save({
                    'epoch': epoch,
                    'model_state_dict': model.state_dict(),
                    'train_loss': train_loss / train_n,
                    'train_acc': train_acc / train_n,
                    'test_loss': valid_loss,
                    'test_acc': valid_acc,
                    'train_parameters': {'batch': args.batch_size, 'epochs': args.epochs, 'lr': lr,
                                         'wd': args.weight_decay}
                }, "./neural_networks/models/" + args.dataset_type + '_' + args.neural_network_type + '_' + args.fname)
            else:
                torch.save({
                    'epoch': epoch,
                    'model_state_dict': model.state_dict(),
                    'optimizer_state_dict': opt.state_dict(),
                    'train_loss': train_loss / train_n,
                    'train_acc': train_acc / train_n,
                    'test_loss': valid_loss,
                    'test_acc': valid_acc,
                    'device': device,
                    'model': model,
                    'train_parameters': {'batch': args.batch_size, 'epochs': args.epochs, 'lr': lr,
                                         'wd': args.weight_decay}
                }, "./neural_networks/models/" + args.dataset_type + '_' + args.neural_network_type + '_' + args.fname)



            print("Saved PyTorch Model State")
        print(f'epoch:{epoch}, train_time:{train_time - start_time}, lr:{lr}, train loss:{train_loss / train_n}, '
              f'train acc {train_acc / train_n}, valid loss:{valid_loss}, valid acc:{valid_acc}')

    model.load_state_dict(torch.load("./neural_networks/models/" + args.dataset_type + '_' + args.neural_network_type + '_' + args.fname, map_location='cpu')['model_state_dict'])
    model.float()
    model.to(device)
    model.eval()
    test_loss, test_acc = evaluate_test_accuracy(test_loader, model, device)
    print(f'FINAL test loss:{test_loss}, test acc:{test_acc}')


if __name__ == "__main__":
    main()
