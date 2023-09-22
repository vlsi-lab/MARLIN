from neural_networks.MNIST.mnist_net import adapt_mnist_net
from neural_networks.CIFAR10.resnet import resnet8, resnet14, resnet20, resnet32, resnet50, resnet56
from neural_networks.utils import (get_loaders_split, evaluate_test_accuracy)
from benchmark_CIFAR10.Utils.utils import adapt_model_9x9
import tikzplotlib
import matplotlib.pyplot as plt
from matplotlib.pyplot import figure
from pathlib import Path
import benchmark_CIFAR10.Utils.GA_utils as GA_utils
import numpy as np
import time
import torch
import pickle
import argparse


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--neural-network', default="resnet8", type=str, help="Choose one from fashionmnist, mnist, resnet8, resnet14, resnet20, resnet32, resnet50, resnet56")
    parser.add_argument('--batch-size', default=100, type=int, help="Number of images processed during each iteration")
    parser.add_argument('--data-dir', default="/data/dataset/pytorch_only/", type=str, help="Directory in which the MNIST and FASHIONMNIST dataset are stored or should be downloaded")
    parser.add_argument('--epochs', default=1, type=int, help="Number of retraining epochs executed for each individual during the NSGA search")
    parser.add_argument('--lr-max', default=1e-2, type=float, help="Maximum learning rate for 'cyclic' scheduler, standard learning rate for 'flat' scheduler")
    parser.add_argument('--lr-min', default=1e-4, type=float, help="Minimum learning rate for 'cyclic' scheduler")
    parser.add_argument('--lr-type', default="cyclic", type=str, help="Select learning rate scheduler, choose between 'cyclic' or 'flat'")
    parser.add_argument('--weight-decay', default=5e-4, type=float, help="Weight decay applied during the optimization step")
    parser.add_argument('--fname', default="baseline_model.pth", type=str, help="Name of the model, must include .pth")
    parser.add_argument('--num-workers', default=2, type=int, help="Number of threads used during the preprocessing of the dataset")
    parser.add_argument('--threads', default=16, type=int, help="Number of threads used during the inference, used only when neural-network-type is set to adapt")
    parser.add_argument('--seed', default=42, type=int, help="Seed for reproducible random initialization")
    parser.add_argument('--lr-momentum', default=0.9, type=float, help="Learning rate momentum")
    parser.add_argument('--split-val', default=0.1, type=float, help="The split-val is used to divide the training set in training and validation with the following dimensions: train=train_images*(1-split_val)  valid=train_images*split_val")
    parser.add_argument('--partial-val', default=10, type=float, help="The partial-val defines the portion of the training set used for partial retraining during the evaluation of each individual. The number of train images is defined as train=train_images*(1-split_val)/partial_val")
    parser.add_argument('--retrain-type', default='full', type=str, help="Defines whether each approximate neural network evaluated during the search is not retrained, retrained entirely or retrained by updating just the bias of convolutional layers. Choose between 'none', 'bias', and 'full'")
    parser.add_argument('--start-from-last', default=False, type=bool, help="Set to true to reload a previous pareto front configuration")
    parser.add_argument('--axx-linear', default=False, type=bool, help="Set to True to enable approximate computing of linear layers. Ignored by default for CIFAR10 networks")
    parser.add_argument('--population', default=70, type=int, help="The number of new individuals evaluated at each iteration. Each individal is an approximate NN")
    parser.add_argument('--generations', default=80, type=int, help="The number of generations explored during the genetic search")
    parser.add_argument('--crossover-probability', default=0.8, type=float, help="Probability of performing a single-point crossover operation on an individual")
    parser.add_argument('--mutation-probability', default=0.8, type=float, help="Probability of gene mutation occurring on an individual")
    parser.add_argument('--axx-levels', default=255, type=int, help="Number of approximation levels supported by the multiplier, or number of LUT corresponding to different multipliers")
    parser.add_argument('--disable-aug', default=True, type=bool, help="Set to True to disable data augmentation to obtain deterministic results")
    parser.add_argument('--just-plot', default=False, type=bool, help="Set to True to generate and plot the graphs without retraining the approximate neural networks. This flag also enables the filtering of Pareto dominated solutions, generating a PDF and a TEX graphs containing only the dominant points.")
    return parser.parse_args()


def main():
    params = get_args()
    train_args = {'epochs': params.epochs, 'lr_min': params.lr_min, 'lr_max': params.lr_max, 'batch': params.batch_size,
                  'weight_decay': params.weight_decay, 'num_workers': params.num_workers, 'lr_momentum': params.lr_momentum}
    model_name = "./neural_networks/models/" + params.neural_network + "_quant_baseline_model.pth"

    torch.set_num_threads(params.threads)

    pkl_repo = './benchmark_CIFAR10/results_pkl/'
    in_file = pkl_repo + params.neural_network + 'pareto_conf_' + str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability * 10)) + '_Pm' + str(
    int(params.mutation_probability * 10)) + '_seed1' + '.pkl'
    f_file = pkl_repo + params.neural_network + 'pareto_res_' + str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability * 10)) + '_Pm' + str(
        int(params.mutation_probability * 10)) + '_seed1' + '.pkl'
    print("loading last pareto front")
    file = open(in_file, 'rb')
    front_conf = pickle.load(file)
    file.close()

    file = open(f_file, 'rb')
    front_valid_res = pickle.load(file)
    file.close()

    imsize = (1,1,28,28) if (params.neural_network == 'mnist' or params.neural_network == 'fashionmnist' ) else (1,3,32,32)
    approximate_linear = True if (params.neural_network == 'mnist' or params.neural_network == "fashionmnist") else False

    if params.neural_network == 'mnist' or params.neural_network == "fashionmnist":
        train_dataloader, valid_dataloader, test_dataloader = get_loaders_split(params.data_dir,
                                                                                batch_size=train_args['batch'],
                                                                                dataset_type=params.neural_network,
                                                                                num_workers=train_args['num_workers'],
                                                                                split_val=params.split_val,
                                                                                disable_aug=params.disable_aug)
    else:
        train_dataloader, valid_dataloader, test_dataloader = get_loaders_split(params.data_dir,
                                                                                batch_size=train_args['batch'],
                                                                                dataset_type='cifar10',
                                                                                num_workers=train_args['num_workers'],
                                                                                split_val=params.split_val,
                                                                                disable_aug=params.disable_aug)

    if params.neural_network == "resnet8":
        model = resnet8('adapt')
    elif params.neural_network == "resnet14":
        model = resnet14('adapt')
    elif params.neural_network == "resnet20":
        model = resnet20('adapt')
    elif params.neural_network == "resnet32":
        model = resnet32('adapt')
    elif params.neural_network == "resnet50":
        model = resnet50('adapt')
    elif params.neural_network == "resnet56":
        model = resnet56('adapt')
    elif params.neural_network == "mnist" or params.neural_network == 'fashionmnist':
        model = adapt_mnist_net()
    else:
        exit("error unknown CNN model name")

    print(model)

    checkpoint = torch.load(model_name, map_location='cpu')
    model.to('cpu')
    model.load_state_dict(checkpoint['model_state_dict'])
    model.to('cpu')

    norm_power_file = './benchmark_CIFAR10/mult_energy.txt'
    power_list = GA_utils.mult_power_list(norm_power_file)
    mult_per_layer = GA_utils.list_mult_per_layer(model, imsize=imsize, axx_linear=approximate_linear)
    print(mult_per_layer)
    tot_mul = 0
    for mul in mult_per_layer:
        tot_mul += mul
    print("Tot MAC operations: ",tot_mul)
    max_power = GA_utils.max_power_net(mult_per_layer, power_list)
    _, test_acc = evaluate_test_accuracy(test_dataloader, model, device='cpu')
    print(f'Baseline test accuracy: {test_acc}\n Baseline energy: {max_power}')

    if params.just_plot:
        f_file = pkl_repo + params.neural_network + 'final_res_' + str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability * 10)) + '_Pm' + str(
            int(params.mutation_probability * 10)) + '_seed1' + '.pkl'
        file = open(f_file, 'rb')
        front_test_res = pickle.load(file)
        file.close()
        Pareto_idx = []

        for i, p in enumerate(front_test_res):  # find all points better than i in f1, and check whether they are better in f2
            C1 = []
            dom = False

            for j, p_j in enumerate(front_test_res):
                if p_j[0] < p[0]:  # all point dominating i on f1
                    C1.append(j)
            if not C1:  # dominating point
                Pareto_idx.append(i)
            else:
                for j in C1:
                    if front_test_res[i][1] < front_test_res[j][1]:
                        dom = True
                if not dom:
                    Pareto_idx.append(i)

        test_ene_par = np.empty(len(Pareto_idx))
        test_acc_par = np.empty(len(Pareto_idx))
        valid_ene_par = np.empty(len(Pareto_idx))
        valid_acc_par = np.empty(len(Pareto_idx))
        for idx, i in enumerate(Pareto_idx):
            test_ene_par[idx] = front_test_res[i][0]
            test_acc_par[idx] = front_test_res[i][1]
            valid_ene_par[idx] = front_valid_res[i][0]
            valid_acc_par[idx] = front_valid_res[i][1]

    else:
        front_test_res = np.zeros((len(front_conf), 2))
        print("eval solution ", len(front_conf))
        start_time = time.time()
        for i in range(len(front_conf)):
            design_time = time.time()
            design = front_conf[i]
            print(design)
            model.load_state_dict(checkpoint['model_state_dict'])
            adapt_model_9x9(model, design, axx_linear=approximate_linear, retrain_type=params.retrain_type)
            front_test_res[i][1] = GA_utils.evaluate_accuracy(model, train_dataloader, test_dataloader, train_args, partial=False, partial_val=params.partial_val, test_only=False)

            front_test_res[i][0] = GA_utils.evaluate_power(mult_per_layer, power_list, max_power, design)
            model.load_state_dict(checkpoint['model_state_dict'])
            print(f"evaluated design {i + 1} out of {len(front_conf)} || energy is: {front_valid_res[i][0]} || pre train test acc is: {front_valid_res[i][1]} || post train test acc is: {front_test_res[i][1]} || time is: {(time.time() - design_time)}")

        total_time = time.time()-start_time
        print(f'Total time: {total_time}, average time per iteration:{total_time/len(front_conf)}')

        f_file = pkl_repo + params.neural_network + 'final_res_' + str(params.generations) + '_' + str(params.population) + '_Pc' + str(
            int(params.crossover_probability * 10)) + '_Pm' + str(
            int(params.mutation_probability * 10)) + '_seed1' + '.pkl'

        file = open(f_file, 'wb')
        pickle.dump(front_test_res, file)
        file.close()


    def tikzplotlib_fix_ncols(obj):
        """
        workaround for matplotlib 3.6 renamed legend's _ncol to _ncols, which breaks tikzplotlib
        """
        if hasattr(obj, "_ncols"):
            obj._ncol = obj._ncols
        for child in obj.get_children():
            tikzplotlib_fix_ncols(child)


    # plot results
    fig = figure(figsize=(13, 8))
    if params.just_plot:
        line1 = plt.plot(valid_acc_par, valid_ene_par, 'rx', linewidth=1.0,
                         label="Dominant solutions Pareto Pre training")
        line2 = plt.plot(test_acc_par, test_ene_par, 'bP', linewidth=1.0, label="Dominant solutions post training")
    else:
        line1 = plt.plot(front_test_res[:, 1], front_test_res[:, 0], 'bP', linewidth=1.0,
                         label="Post Training Test results NSGA")
        line2 = plt.plot(front_valid_res[:, 1], front_valid_res[:, 0], 'gx', linewidth=1.0,
                         label="Post Training Validation results NSGA")
    plt.xticks(fontsize=14)
    plt.yticks(fontsize=14)
    # plt.legend(loc="upper right", fontsize= 12)
    plt.title("Accuracy vs normalized energy: " + params.neural_network)
    plt.xlabel("Accuracy", fontsize=12)
    plt.ylabel("Normalized energy", fontsize=12)
    plt.legend(loc='upper left')

    Path('./benchmark_CIFAR10/results_fig/').mkdir(parents=True, exist_ok=True)
    plt.grid(True)
    if params.just_plot:
        plt.savefig('./benchmark_CIFAR10/results_fig/' + params.neural_network + '_accuracy_vs_power' + '_Ng' + str(params.generations) + '_Np' + str(params.population) + '_Pc' + str(params.crossover_probability) + '_Pm' + str(params.mutation_probability) + '_dominant.pdf')

        tikzplotlib_fix_ncols(fig)
        tikzplotlib.save('./benchmark_CIFAR10/results_fig/' + params.neural_network + '_accuracy_vs_power' + '_Ng' + str(params.generations) + '_Np' + str(params.population) + '_Pc' + str(params.crossover_probability) + '_Pm' + str(params.mutation_probability) + "_dominant.tex")
    else:
        plt.savefig('./benchmark_CIFAR10/results_fig/' + params.neural_network + '_accuracy_vs_power' + '_Ng' + str(params.generations) + '_Np' + str(params.population) + '_Pc' + str(params.crossover_probability) + '_Pm' + str(params.mutation_probability) + '.pdf')

        tikzplotlib_fix_ncols(fig)
        tikzplotlib.save('./benchmark_CIFAR10/results_fig/' + params.neural_network + '_accuracy_vs_power' + '_Ng' + str(params.generations) + '_Np' + str(params.population) + '_Pc' + str(params.crossover_probability) + '_Pm' + str(params.mutation_probability) + ".tex")
    plt.show()


if __name__ == "__main__":
    main()

