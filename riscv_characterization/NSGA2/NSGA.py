from pymoo.optimize import minimize
from Utils.torch_neural_networks_library import *
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
import Utils.GA_utils as GA_utils
import numpy as np
import time

def get_args():
    # TODO: uniformare questo parser con quello che serve qua
    parser = argparse.ArgumentParser()
    parser.add_argument('--neural-network', default="resnet32", type=str, help="Choose one from fashionmnist, mnist, resnet8, resnet14, resnet20, resnet32, resnet50, resnet56")
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
    parser.add_argument('--generations', default=120, type=int, help="The number of generations explored during the genetic search")
    parser.add_argument('--crossover-probability', default=0.8, type=float, help="Probability of performing a single-point crossover operation on an individual")
    parser.add_argument('--mutation-probability', default=0.8, type=float, help="Probability of gene mutation occurring on an individual")
    parser.add_argument('--axx-levels', default=255, type=int, help="Number of approximation levels supported by the multiplier, or number of LUT corresponding to different multipliers")
    parser.add_argument('--disable-aug', default=True, type=bool, help="Set to True to disable data augmentation to obtain deterministic results")

    return parser.parse_args()

# Valutare se incorporare o rendere uguale al mio come contorno
class ProblemWrapper(Problem):
    def _evaluate(self, designs, out, *args, **kwargs):
        res1 = []
        res2 = []
        for design in designs:
            # design is an individual, f1 and f2 are fitness functions to optimize
            f1 = GA_utils.evaluate_power(mult_per_layer, power_list, max_power, design)
            f2 = 1.0/GA_utils.evaluate_accuracy(model, design, None, None)
            res1.append(f1)
            res2.append(f2)
        out['F'] = np.column_stack([ np.array(res1), np.array(res2)])

# start time
start_time = time.time()

# define files and model to use
model = MNIST_model1()  # create model instance, initialize parameters, send to device
norm_power_file = './NSGA2_compile_ultra/normalized_power_27s_compile_ultra9x9_UMC65.txt' # normalized power txt file

# define NSGA-II parameters
"""
- Np is the population size
- Ng is the number of generations
- Pc is the crossover probability
- Pm is the mutation probability
"""
Np = 10
Ng = 10
Pc = 1.0
Pm = 0.3
## define the problem
n_appr_levels, n_levels = GA_utils.encode_chromosome(model)
xl = 0  #lista di n_levels elementi, contiene il lower limit di ogni variabile,
#sono n_levels moltiplicatori, quindi n_levels variabili con valori da 0 a 255
xu = 255    #lista di n_levels elementi, contiene l'upper limit di ogni variabile,
mult_per_layer = GA_utils.list_mult_per_layer(model, None)
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
    seed = 1, 
    termination = stop_criteria
)

print("--- %s seconds ---" % (time.time() - start_time))

# save results to txt files
txt_repo = './NSGA2_compile_ultra/results_27s_ultra_txt/'
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
