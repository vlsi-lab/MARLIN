# CIFAR-1O benchmark

**Before running any of these scripts you must read the README inside the folder "neural networs" or you will not be able to reproduce the experimental results**

This folder contains 3 python files that are used to explore the approximate NNs search space, and are detailed in the next sections. The results provided in "/results_fig" and "results_pkl" are those presented in the research paper and can be used to generate the configuration utilization with "plot_counts.py" or the energy/accuracy graphs with "evaluate_final_front.py"
In order to use these scripts, it is necessary to activate the conda environment "marlin".
> conda activate marlin

## nsga.py
This script wraps the approximate NNs search with the genetic agent NSGA-II. In its default state, the script will execute the search with ResNet-8 and with the default hyper-parameters. There are several options that can be set by the user, it is possible to visualize them by usin the help command as follows:

> python benchmark_CIFAR10/nsga.py --help

To replicate the results for the six ResNet models presented in the paper, using the provided pre-trained NNs, the commands are the following:
> python benchmark_CIFAR10/nsga.py --neural-network resnet8 --generations 80
>
> python benchmark_CIFAR10/nsga.py --neural-network resne14 --generations 80
>
> python benchmark_CIFAR10/nsga.py --neural-network resnet20 --generations 80
>
> python benchmark_CIFAR10/nsga.py --neural-network resnet32 --generations 120
>
> python benchmark_CIFAR10/nsga.py --neural-network resnet50 --generations 120
> 
> python benchmark_CIFAR10/nsga.py --neural-network resnet56 --generations 120



## evaluate_final_front.py
This script is used to retrain the Pareto front generated with "nsga.py".
The folders "results_pkl" and "results_fig" already contain the .pkl, .pdf and .tex files produced by this script, which include the results presented in the paper. You can reproduce these results by executing the following commands. This script will take the quantized NN weights and assign the approximate configuration found with "nsga.py" and then retrain the NN for one epoch. Before this retraining step, the NN model has no knowledge of the optimal multiplier assignment that was done during the genetic search, therefore, the retraining is necessary to obtain the final accuracy.

> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resnet8 --generations 80 --just-plot False
>
> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resne14 --generations 80 --just-plot False
>
> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resnet20 --generations 80 --just-plot False
>
> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resnet32 --generations 120 --just-plot False
>
> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resnet50 --generations 120 --just-plot False
> 
> python benchmark_CIFAR10/evaluate_final_front.py --neural-network resnet56 --generations 120 --just-plot False

After having trained the Pareto front from the genetic search, some solutions may have become dominant. To find them, execute the previous commands changing the "--just-plot" flag to True.

## plot_counts.py
This script will simply plot the utilization occurrence of each approximate configuration of the reconfigurable multiplier included with this repository, using the experimental data provided in "results_pkl". There are no parameters for this script, as it is just used to generate the graph presented in the paper.


