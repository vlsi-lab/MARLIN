from pymoo.optimize import minimize
from neural_networks.MNIST.mnist_net import adapt_mnist_net
from neural_networks.CIFAR10.resnet import resnet8, resnet14, resnet20, resnet32, resnet50, resnet56
from neural_networks.utils import (get_loaders_split, evaluate_test_accuracy)
from pymoo.algorithms.moo.nsga2 import NSGA2
from benchmark_CIFAR10.Utils.utils import adapt_model_9x9
from pymoo.core.problem import Problem
from pymoo.operators.sampling.rnd import IntegerRandomSampling
from pymoo.operators.repair.rounding import RoundingRepair
from pymoo.operators.crossover.pntx import SinglePointCrossover
from pymoo.operators.mutation.pm import PM
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
    parser.add_argument('--num-workers', default=4, type=int, help="Number of threads used during the preprocessing of the dataset")
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

    return parser.parse_args()


def main():
    params = get_args()

    train_args = {'epochs': params.epochs, 'lr_min': params.lr_min, 'lr_max': params.lr_max, 'batch': params.batch_size,
                  'weight_decay': params.weight_decay, 'num_workers': params.num_workers, 'lr_momentum': params.lr_momentum}
    model_name = "./neural_networks/models/" + params.neural_network + "_quant_baseline_model.pth"

    torch.set_num_threads(params.threads)

    pkl_repo = './benchmark_CIFAR10/results_pkl/'
    Path(pkl_repo).mkdir(parents=True, exist_ok=True)
    in_file = pkl_repo + params.neural_network +'pareto_conf_'+ str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability*10)) +'_Pm' + str(int(params.mutation_probability*10)) + '_seed1' + '.pkl'
    f_file = pkl_repo + params.neural_network + 'pareto_res_' + str(params.generations) +'_' + str(params.population) +'_Pc' + str(int(params.crossover_probability*10)) +'_Pm' + str(int(params.mutation_probability*10)) +'_seed1'+ '.pkl'

    imsize = (1,1,28,28) if (params.neural_network == 'mnist' or params.neural_network == 'fashionmnist' ) else (1,3,32,32)
    if params.start_from_last:
        print("loading last pareto front")
        file = open(pkl_repo + params.neural_network +'_backup_'+ str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability*10)) +'_Pm' + str(int(params.mutation_probability*10)) + '_seed1' + '.pkl', 'rb')
        last_design = pickle.load(file)
        file.close()

    class ProblemWrapper(Problem):
        def _evaluate(self, designs, out, *args, **kwargs):
            res1 = []
            res2 = []
            print("eval solution ", len(designs))
            if self.data['current_gen'] == 0 and params.start_from_last:

                for i in range(len(last_design['designs'])):
                    designs[i] = last_design['designs'][i]

            print(f"current gen is {self.data['current_gen']+1} out of {params.generations}")
            file = open(pkl_repo + params.neural_network +'_backup_'+ str(params.generations) + '_' + str(params.population) + '_Pc' + str(int(params.crossover_probability*10)) +'_Pm' + str(int(params.mutation_probability*10)) + '_seed1' + '.pkl', 'wb')
            pickle.dump({'designs':designs, 'current_gen':self.data['current_gen'], 'Ng':params.generations, 'Np':params.population, 'Pc':params.crossover_probability, 'Pm':params.mutation_probability, 'net':params.neural_network}, file)
            file.close()
            for i in range(len(designs)):
                design_time = time.time()
                design = designs[i]
                design_id = "".join([str(a) for a in design])
                if design_id in self.data['previous_designs_results']:
                    # design already evaluated
                    f1 = self.data['previous_designs_results']['design_id']['f1']
                    f2 = self.data['previous_designs_results']['design_id']['f2']
                    res1.append(f1)
                    res2.append(f2)
                    print("skipped")
                else:
                    self.data['previous_designs_results']['design_id'] = {}
                    # print(design)
                    checkpoint = torch.load(model_name, map_location='cpu')
                    model.load_state_dict(checkpoint['model_state_dict'])
                    adapt_model_9x9(model, design, axx_linear=params.axx_linear, retrain_type='full')  # model becomes adapt model
                    # design is an individual, fitness function is the function to optimize
                    f1 = GA_utils.evaluate_power(mult_per_layer, power_list, max_power, design)
                    f2 = 1/GA_utils.evaluate_accuracy(model, train_dataloader, valid_dataloader, train_args, test_only=False, partial=True, partial_val=params.partial_val)
                    self.data['previous_designs_results']['design_id']['f1'] = f1
                    self.data['previous_designs_results']['design_id']['f2'] = f2
                res1.append(f1)
                res2.append(f2)

                # print(f"evaluated design {i+1} out of {len(designs)}, time is {(time.time() - design_time)}")
                print(f"design: {i + 1}/{len(designs)} || gen: {self.data['current_gen']+1}/{params.generations} || energy is: {(100*f1):0.2f} || valid acc is: {(100/f2):0.2f}|| time is: {(time.time() - design_time):0.2f}")


            out['F'] = np.column_stack([ np.array(res1), np.array(res2)])
            self.data['current_gen'] += 1
    start_time = time.time()

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

    checkpoint = torch.load(model_name, map_location='cpu')
    model.to('cpu')
    model.load_state_dict(checkpoint['model_state_dict'])
    model.to('cpu')

    norm_power_file = './benchmark_CIFAR10/mult_energy.txt'

    n_appr_levels, n_levels = GA_utils.encode_chromosome(model, axx_linear=params.axx_linear, n_appr_levels=params.axx_levels)
    print(n_appr_levels, n_levels)
    xl = 0  
    mult_per_layer = GA_utils.list_mult_per_layer(model, imsize=imsize, axx_linear=params.axx_linear)

    power_list = GA_utils.mult_power_list(norm_power_file)
    max_power = GA_utils.max_power_net(mult_per_layer, power_list)

    test_loss, test_acc = evaluate_test_accuracy(test_dataloader, model, device='cpu')
    print(f'Baseline test accuracy: {test_acc}')
    problem = ProblemWrapper(n_var=n_levels, n_obj=2, xl=xl, xu=params.axx_levels, vtype=int, mult_per_layer=mult_per_layer, power_list=power_list, max_power=max_power, model=model, max_acc=test_acc, current_gen=0, previous_designs_results={})

    algorithm = NSGA2(pop_size=params.population, sampling=IntegerRandomSampling(),
                      crossover=SinglePointCrossover(prob=params.crossover_probability, repair=RoundingRepair()),
                      mutation=PM(prob=params.mutation_probability, eta=3.0, vtype=float, repair=RoundingRepair()),)

    stop_criteria = ('n_gen', params.generations)

    results = minimize(problem=problem, algorithm=algorithm, seed=1, termination=stop_criteria)

    print("--- %s seconds ---" % (time.time() - start_time))

    file = open(in_file, 'wb')
    pickle.dump(results.X, file)
    file.close()

    file = open(f_file, 'wb')
    pickle.dump(results.F, file)
    file.close()


if __name__ == "__main__":
    main()

