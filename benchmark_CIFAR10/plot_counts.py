import tikzplotlib
import matplotlib.pyplot as plt
from matplotlib.pyplot import figure
from pathlib import Path
import numpy as np
import pickle


def tikzplotlib_fix_ncols(obj):
    """
    workaround for matplotlib 3.6 renamed legend's _ncol to _ncols, which breaks tikzplotlib
    """
    if hasattr(obj, "_ncols"):
        obj._ncol = obj._ncols
    for child in obj.get_children():
        tikzplotlib_fix_ncols(child)


def main():
    levels = 256
    
    pkl_repo = './benchmark_CIFAR10/results_pkl/'
    
    in_file = pkl_repo + 'resnet8pareto_conf_80_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf8 = pickle.load(file)
    file.close()
    
    in_file = pkl_repo + 'resnet14pareto_conf_80_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf14 = pickle.load(file)
    file.close()
    
    in_file = pkl_repo + 'resnet20pareto_conf_80_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf20 = pickle.load(file)
    file.close()
    
    in_file = pkl_repo + 'resnet32pareto_conf_120_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf32 = pickle.load(file)
    file.close()

    in_file = pkl_repo + 'resnet50pareto_conf_120_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf50 = pickle.load(file)
    file.close()

    in_file = pkl_repo + 'resnet56pareto_conf_120_70_Pc8_Pm8_seed1.pkl'
    file = open(in_file, 'rb')
    front_conf56 = pickle.load(file)
    file.close()
    
    count_conf8 = np.zeros(levels)
    count_conf14 = np.zeros(levels)
    count_conf20 = np.zeros(levels)
    count_conf32 = np.zeros(levels)
    count_conf50 = np.zeros(levels)
    count_conf56 = np.zeros(levels)
    conf_index = np.arange(0, levels)
    
    tot = 0
    for conf in front_conf8:
        for lay in conf:
            count_conf8[lay] += 1
            tot += 1
    count_conf8 = count_conf8 / tot * 100
    
    tot = 0
    for conf in front_conf14:
        for lay in conf:
            count_conf14[lay] += 1
            tot += 1
    count_conf14 = count_conf14 / tot * 100

    tot = 0
    for conf in front_conf20:
        for lay in conf:
            count_conf20[lay] += 1
            tot += 1
    count_conf20 = count_conf20 / tot * 100

    tot = 0
    for conf in front_conf32:
        for lay in conf:
            count_conf32[lay] += 1
            tot += 1
    count_conf32 = count_conf32 / tot * 100
    
    tot = 0
    for conf in front_conf50:
        for lay in conf:
            count_conf50[lay] += 1
            tot += 1
    count_conf50 = count_conf50 / tot * 100
    
    tot = 0
    for conf in front_conf56:
        for lay in conf:
            count_conf56[lay] += 1
            tot += 1
    count_conf56 = count_conf56 / tot * 100
    
    fig = figure(figsize=(13, 8))
    
    width = 0.2
    
    plt.bar(conf_index, count_conf8, width=width, label="ResNet 8")
    plt.bar(conf_index + width, count_conf14, width=width, label="ResNet 14")
    plt.bar(conf_index + 2 * width, count_conf20, width=width, label="ResNet 20")
    plt.bar(conf_index + 3 * width, count_conf32, width=width, label="ResNet 32")
    plt.bar(conf_index + 4 * width, count_conf50, width=width, label="ResNet 50")
    plt.bar(conf_index + 5 * width, count_conf56, width=width, label="ResNet 56")
    
    plt.xticks(fontsize=14)
    plt.yticks(fontsize=14)
    plt.title("Approximate multiplier configuration occurrences")
    plt.xlabel("Approximation level", fontsize=12)
    plt.ylabel("Occurrence [%]", fontsize=12)
    plt.legend()
    Path('./benchmark_CIFAR10/results_fig/').mkdir(parents=True, exist_ok=True)
    plt.grid(True)
    plt.savefig('./benchmark_CIFAR10/results_fig/all_config_count.pdf')
    
    tikzplotlib_fix_ncols(fig)
    tikzplotlib.save('./benchmark_CIFAR10/results_fig/all_config_count.tex')
    plt.show()
    
    higher = np.argmax(count_conf8)
    print(higher, count_conf8[higher])
    
    high8 = []
    high14 = []
    high20 = []
    high32 = []
    high50 = []
    high56 = []
    
    for i in range(5):
        higher = np.argmax(count_conf8)
        high8.append([higher, round(count_conf8[higher], 2)])
        count_conf8[higher] = 0
        higher = np.argmax(count_conf14)
        high14.append([higher, round(count_conf14[higher], 2)])
        count_conf14[higher] = 0
        higher = np.argmax(count_conf20)
        high20.append([higher, round(count_conf20[higher], 2)])
        count_conf20[higher] = 0
        higher = np.argmax(count_conf32)
        high32.append([higher, round(count_conf32[higher], 2)])
        count_conf32[higher] = 0
        higher = np.argmax(count_conf50)
        high50.append([higher, round(count_conf50[higher], 2)])
        count_conf50[higher] = 0
        higher = np.argmax(count_conf56)
        high56.append([higher, round(count_conf56[higher], 2)])
        count_conf56[higher] = 0
    print(f'resnet8 config count {high8}')
    print(f'resnet14 config count {high14}')
    print(f'resnet20 config count {high20}')
    print(f'resnet32 config count {high32}')
    print(f'resnet50 config count {high50}')
    print(f'ResNet56 config count {high56}')


if __name__ == "__main__":
    main()

