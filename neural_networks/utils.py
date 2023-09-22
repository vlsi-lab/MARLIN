import torch
import torch.nn.functional as F
from torchvision import datasets, transforms
from torch.utils.data.sampler import SubsetRandomSampler


def evaluate_test_accuracy(test_loader, model, device="cuda"):
    """
    Evaluate the model top-1 accuracy using the dataloader passed as argument
    @param test_loader: dataloader that contains the test images and ground truth, could be either the test or validation split
    @param model: any torch.nn.Module with a final dense layer
    @param device: use "cpu" for adapt with approximate models, "cuda" for GPU for float or quantized baseline models
    @return: return the test loss and test accuracy as floating points values
    """
    test_loss = 0
    test_acc = 0
    n = 0
    model.eval()
    with torch.no_grad():
        for i, (X, y) in enumerate(test_loader):
            X, y = X.to(device), y.to(device)
            output = model(X)
            loss = F.cross_entropy(output, y)
            test_loss += loss.item() * y.size(0)
            test_acc += (output.max(1)[1] == y).sum().item()
            n += y.size(0)
    return test_loss/n, test_acc/n


cifar10_mean = (0.4914, 0.4822, 0.4465)
cifar10_std = (0.2471, 0.2435, 0.2616)
fashionmnist_mean = 0.286
fasnionmnist_std = 0.353
mnist_mean = 0.1307
mnist_std = 0.3081


def get_loaders(dir_, batch_size, dataset_type, num_workers=2, disable_shuffle=False, is_integer=False):
    """
    Generate train and test dataloaders
    @param dir_: string, directory in which the CIFAR-10 dataset is stored
    @param batch_size: int, dimension of the batch
    @param dataset_type: str, dataset type can be "cifar10", "mnist" or "fashionmnist"
    @param num_workers: int, number of workers used for the pre-processing of the dataset, min is 0 max is the thread count of the CPU
    @param disable_shuffle: disable randomization of the image/label indices, se to False in order to process the same data in the same order for reproducibility
    @param is_integer: normalization done for integer activations, must beset to True only for NEMO processing
    @return: returns the dataloaders
    """
    if is_integer:
        int_mean = -0.5
    else:
        int_mean = 0

    if dataset_type == "cifar10":

        train_transform = transforms.Compose([
            transforms.RandomCrop(32, padding=4),
            transforms.RandomHorizontalFlip(),
            transforms.ToTensor(),
            transforms.Normalize(cifar10_mean, cifar10_std),
            transforms.Normalize(int_mean, 1)
        ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(cifar10_mean, cifar10_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.CIFAR10(dir_, train=True, transform=train_transform, download=True)
        test_dataset = datasets.CIFAR10(dir_, train=False, transform=test_transform, download=True)
    elif dataset_type == "fashionmnist":
        train_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(fashionmnist_mean, fasnionmnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(fashionmnist_mean, fasnionmnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.FashionMNIST(dir_, train=True, transform=train_transform, download=True)
        test_dataset = datasets.FashionMNIST(dir_, train=False, transform=test_transform, download=True)

    elif dataset_type == "mnist":
        train_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(mnist_mean, mnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(mnist_mean, mnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.MNIST(dir_, train=True, transform=train_transform, download=True)
        test_dataset = datasets.MNIST(dir_, train=False, transform=test_transform, download=True)

    else:
        exit("unknown dataset type")

    train_loader = torch.utils.data.DataLoader(
        dataset=train_dataset,
        batch_size=batch_size,
        shuffle=not (disable_shuffle),
        pin_memory=False,
        num_workers=num_workers,
    )
    test_loader = torch.utils.data.DataLoader(
        dataset=test_dataset,
        batch_size=batch_size,
        shuffle=False,
        pin_memory=False,
        num_workers=num_workers,
    )
    return train_loader, test_loader


def get_loaders_split(dir_, batch_size, dataset_type, num_workers=2, split_val=0.2, disable_aug=False, is_integer=False):
    """
    Generate train and test dataloaders
    @param dir_: string, directory in which the CIFAR-10 dataset is stored
    @param batch_size: int, dimension of the batch
    @param dataset_type: str, dataset type can be "cifar10", "mnist" or "fashionmnist"
    @param num_workers: int, number of workers used for the pre-processing of the dataset, min is 0 max is the thread count of the CPU
    @param split_val: float, number between 0 and 1 used to split the training set in training/validation, the validation set has a dimension of split_val*training_set
    @param disable_aug: disable data augmentation, se to False in order to process the same data for reproducibility
    @param is_integer: normalization done for integer activations, must beset to True only for NEMO processing
    @return: returns the dataloaders
    """
    if is_integer:
        int_mean = -0.5
    else:
        int_mean = 0

    if dataset_type == "cifar10":
        if disable_aug:
            train_transform = transforms.Compose([
                transforms.ToTensor(),
                transforms.Normalize(cifar10_mean, cifar10_std),
                transforms.Normalize(int_mean, 1)
            ])
        else:
            train_transform = transforms.Compose([
                transforms.RandomCrop(32, padding=4),
                transforms.RandomHorizontalFlip(),
                transforms.ToTensor(),
                transforms.Normalize(cifar10_mean, cifar10_std),
                transforms.Normalize(int_mean, 1)
            ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(cifar10_mean, cifar10_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.CIFAR10(dir_, train=True, transform=train_transform, download=True)
        val_dataset = datasets.CIFAR10(dir_, train=True, transform=test_transform, download=True)
        test_dataset = datasets.CIFAR10(dir_, train=False, transform=test_transform, download=True)
    elif dataset_type == "fashionmnist":
        train_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(fashionmnist_mean, fasnionmnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(fashionmnist_mean, fasnionmnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.FashionMNIST(dir_, train=True, transform=train_transform, download=True)
        val_dataset = datasets.FashionMNIST(dir_, train=True, transform=train_transform, download=True)
        test_dataset = datasets.FashionMNIST(dir_, train=False, transform=test_transform, download=True)

    elif dataset_type == "mnist":
        train_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(mnist_mean, mnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        test_transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Normalize(mnist_mean, mnist_std),
            transforms.Normalize(int_mean, 1)
        ])
        train_dataset = datasets.MNIST(dir_, train=True, transform=train_transform, download=True)
        val_dataset = datasets.MNIST(dir_, train=True, transform=train_transform, download=True)
        test_dataset = datasets.MNIST(dir_, train=False, transform=test_transform, download=True)

    else:
        exit("unknown dataset type")

    # Evaluation of indices for training and validation splits
    dataset_size = len(train_dataset)
    indices = list(range(dataset_size))
    split = int((split_val * dataset_size))
    train_indices, val_indices = indices[split:], indices[:split]

    # Train and validation samplers
    train_sampler = SubsetRandomSampler(train_indices)
    valid_sampler = SubsetRandomSampler(val_indices)

    train_loader = torch.utils.data.DataLoader(
        dataset=train_dataset,
        batch_size=batch_size,
        shuffle=False,
        pin_memory=False,
        sampler=train_sampler,
        num_workers=num_workers,
    )

    valid_loader = torch.utils.data.DataLoader(
        dataset=val_dataset,
        batch_size=batch_size,
        shuffle=False,
        pin_memory=False,
        sampler=valid_sampler,
        num_workers=num_workers,
    )

    test_loader = torch.utils.data.DataLoader(
        dataset=test_dataset,
        batch_size=batch_size,
        shuffle=False,
        pin_memory=False,
        num_workers=num_workers,
    )

    return train_loader, valid_loader, test_loader

