from pymoo.optimize import minimize
import torch
#from Utils.torch_neural_networks_library import *
from pymoo.algorithms.moo.nsga2 import NSGA2
from pymoo.core.problem import Problem
from pymoo.operators.sampling.rnd import IntegerRandomSampling
from pymoo.operators.repair.rounding import RoundingRepair
from pymoo.operators.crossover.sbx import SBX
from pymoo.operators.crossover.pntx import SinglePointCrossover
from pymoo.operators.mutation.pm import PM
from pymoo.operators.mutation.bitflip import BitflipMutation
import matplotlib.pyplot as plt
from matplotlib.pyplot import figure
from pathlib import Path
from neural_networks.utils import get_loaders_split
from neural_networks.MNIST.mnist_net import MNIST_model1
import nemo_training_quantization.utils as nemo_utils
import benchmark_CIFAR10.Utils.GA_utils as GA_utils
import argparse
import numpy as np
import time

def get_args():
    # TODO: uniformare questo parser con quello che serve qua
    parser = argparse.ArgumentParser()
    parser.add_argument('--batch-size', default=32, type=int, help="Number of images processed during each iteration")
    parser.add_argument('--data-dir', default="/data/dataset/pytorch_only/", type=str, help="Directory in which the MNIST and FASHIONMNIST dataset are stored or should be downloaded")
    parser.add_argument('--num-workers', default=4, type=int, help="Number of threads used during the preprocessing of the dataset")
    parser.add_argument('--split-val', default=0.1, type=float, help="The split-val is used to divide the training set in training and validation with the following dimensions: train=train_images*(1-split_val)  valid=train_images*split_val")
    parser.add_argument('--disable-aug', default=True, type=bool, help="Set to True to disable data augmentation to obtain deterministic results")
    parser.add_argument('--seed', default=1, type=int, help="Seed for reproducible random initialization")
    parser.add_argument('--population', default=3, type=int, help="The number of new individuals evaluated at each iteration. Each individal is an approximate NN")
    parser.add_argument('--generations', default=3, type=int, help="The number of generations explored during the genetic search")
    parser.add_argument('--crossover-probability', default=1.0, type=float, help="Probability of performing a single-point crossover operation on an individual")
    parser.add_argument('--mutation-probability', default=0.3, type=float, help="Probability of gene mutation occurring on an individual")
    parser.add_argument('--axx-levels', default=255, type=int, help="Number of approximation levels supported by the multiplier, or number of LUT corresponding to different multipliers")
    parser.add_argument('--model-file', default='./riscv_characterization/NSGA2/mnist_model.pth', type=str, help="Trained model passed through NEMO")
    parser.add_argument('--json-file', default='./riscv_characterization/NSGA2/act.json', type=str, help="File storing the activation scaling factors, taken from NEMO")
    parser.add_argument('--power-file', default='./riscv_characterization/NSGA2/power.txt', type=str, help="File reporting normalized power values for each approx level")
    parser.add_argument('--plot', default=False, type=bool, help="Set to true for plotting results")
    return parser.parse_args()

def main():
    # Valutare se incorporare o rendere uguale al mio come contorno
    class ProblemWrapper(Problem):
        def _evaluate(self, designs, out, *args, **kwargs):
            res1 = []
            res2 = []
            for design in designs:
                # design is an individual, f1 and f2 are fitness functions to optimize
                f1 = GA_utils.evaluate_power(mult_per_layer, power_list, max_power, design)
                # Reload and transform model to adapt-nemo compatible one before test
                nemo_utils.transform_model(model, model_file, json_file, design)
                f2 = 1.0/GA_utils.evaluate_accuracy(model, train_data=None, test_data=test_dataloader, train_args=None, test_only=True, 
                                                    partial=False, partial_val=0,
                                                    nemo=True, device='cpu')
                res1.append(f1)
                res2.append(f2)
            out['F'] = np.column_stack([ np.array(res1), np.array(res2)])



    params = get_args()
    model_file = params.model_file
    json_file = params.json_file
    # start time
    start_time = time.time()

    _, _, test_dataloader = get_loaders_split(  params.data_dir,
                                                batch_size=params.batch_size,
                                                dataset_type='mnist',
                                                num_workers=params.num_workers,
                                                split_val=params.split_val,
                                                disable_aug=params.disable_aug,
                                                is_integer=True)

    # define files and model to use
    model = MNIST_model1()  # create model instance, initialize parameters, send to device
    norm_power_file = params.power_file # normalized power txt file

    # define NSGA-II parameters
    """
    - Np is the population size
    - Ng is the number of generations
    - Pc is the crossover probability
    - Pm is the mutation probability
    """
    Np = params.population
    Ng = params.generations
    Pc = params.crossover_probability
    Pm = params.mutation_probability
    ## define the problem
    _, n_levels = GA_utils.encode_chromosome(model, params.axx_levels)
    xl = 0 # lower bound
    xu = params.axx_levels # upper bound
    mult_per_layer = GA_utils.list_mult_per_layer(model, imsize=(1,1,28,28))
    print(mult_per_layer)
    power_list = GA_utils.mult_power_list(norm_power_file)
    max_power = GA_utils.max_power_net(mult_per_layer, power_list)

    problem = ProblemWrapper(n_var = n_levels, n_obj = 2, xl = xl,xu = xu, vtype=int, mult_per_layer=mult_per_layer, power_list=power_list, max_power=max_power, model=model)


    algorithm = NSGA2(pop_size=Np,
            sampling=IntegerRandomSampling(),
            crossover=SinglePointCrossover(prob=Pc, repair=RoundingRepair()),
            mutation=PM(prob=Pm,eta=3.0, vtype=float, repair=RoundingRepair()),) #here can change other probabilities

    stop_criteria = ('n_gen', Ng)


    results = minimize (
        problem = problem,
        algorithm = algorithm,
        seed = params.seed, 
        termination = stop_criteria
    )

    print("--- %s seconds ---" % (time.time() - start_time))

    # save results to txt files
    txt_repo = './results_txt/'
    Path(txt_repo).mkdir(parents=True, exist_ok=True)

    in_file = txt_repo +'pareto_conf_'+ str(Ng) + '_' + str(Np) + '_Pc' + str(Pc) +'_Pm' + str(Pm) + '_seed1' + '.txt'
    f_file = txt_repo + 'pareto_res_' + str(Ng) +'_' + str(Np) +'_Pc' + str(Pc) +'_Pm' + str(Pm) +'_seed1'+'.txt'
    in_f = open(in_file, 'w')
    f_f = open(f_file, 'w')

    for inputs in results.X:
        #print("inputs", inputs)
        in_f.write(str(inputs) +'\n')
    in_f.close()

    for fitness in results.F:
        f_f.write(str(fitness) +'\n')   # power, 1/accuracy
    f_f.close()

    if(params.plot):
        # plot results
        res_data = results.F.T
        figure(figsize=(13,8))
        line1 = plt.plot(1.0/res_data[1], res_data[0],'bo', linewidth=1.0)

        plt.xticks(fontsize=14)
        plt.yticks(fontsize=14)
        #plt.legend(loc="upper right", fontsize= 12)
        plt.title("Accuracy vs normalized dynamic power : ")
        plt.xlabel("Accuracy", fontsize=12)
        plt.ylabel("Nromalized Dynamic Power", fontsize=12)

        Path('plot_27s').mkdir(parents=True, exist_ok=True)
        plt.grid (True)
        #plt.savefig('./plot_27s_ultra/'+'accuracy_vs_power'+ str(Ng) +'_'+str(Np) +'_Pc' + str(Pc) +'_Pm' + str(Pm) + '_seed1' + '.pdf')
        plt.show()

if __name__ == "__main__":
    main()