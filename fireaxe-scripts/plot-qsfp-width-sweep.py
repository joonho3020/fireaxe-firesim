#!/usr/bin/env python3

import os
import sys
import math
import argparse
import csv
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.patches import FancyBboxPatch
import numpy as np
import re
from typing import Dict, List, Tuple


matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

parser = argparse.ArgumentParser(description='Draw qsfp interface width perf sweep result (Fig. 11)')
parser.add_argument('--input-dir', type=str, required=True, help='path to input directory containing the output csv files from FireSim runs')
parser.add_argument('--output-dir', type=str, default='generated-plots', help='path to output directory to place the image')
parser.add_argument("--figsize-x", type=float, default=24.0, help="")
parser.add_argument("--figsize-y", type=float, default=14.0, help="")
parser.add_argument("--dpi", type=float, default=300.0, help="")
parser.add_argument("--fontsize", type=int, default=30, help="")
args = parser.parse_args()

figsize = (args.figsize_x, args.figsize_y)
fontsize = args.fontsize
linewidth=3
markersize=15

str2int = {
    'one'     : 1,
    'two'     : 2,
    'four'    : 4,
    'eight'   : 8,
    'sixteen' : 16
}

numtiles2width = {
    1:  618,
    2:  1236,
    4:  2476,
    8:  4960,
    16: 9936
}

                    # mode  freq      width  perf
Data = Dict[Tuple[str, int], Dict[int, float]]

def parse_input_csv_files() -> Data:
    files = [f for f in os.listdir(args.input_dir) if re.match(r'qsfp.*-[0-9]+\.csv', f)]
    print(files)

    data : Data = dict()
    for file in files:
        (filename, _) = os.path.splitext(file)

        words = filename.split('-')
        mode = words[1]
        target_freq = int(words[2])

        width_to_freq: Dict[int, float] = dict()
        with open(os.path.join(args.input_dir, file), 'r') as f:
            first_line = True
            for line in csv.reader(f):
                if first_line:
                    first_line = False
                    continue

                num_tiles = int(math.log2(str2int[line[0].split('-')[-1]]))
                sim_freq_mhz = float(line[4]) / 1000
                width_to_freq[num_tiles] = sim_freq_mhz
        data[(mode, target_freq)] = dict(sorted(width_to_freq.items()))
    return data

def plot(data: Data):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    print(fig.axes)
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Partition Interface Width (bits)', fontsize=fontsize)
    ax.set_ylabel('Simulation Frequency (MHz)',       fontsize=fontsize)

    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    ax.set_ylim(0.0, 2.4)
    ax.set_yticks(ticks=[i * 0.25 for i in range(10)])

    ax.set_xticks(ticks=[i for i in range(5)])
    ax.set_xticks(ticks=[i + 0.5 for i in range(0, 4)], minor=True)
    ax.set_xticklabels([618, 1236, 2476, 4960, 9936])

    ax.grid(axis="x", which='minor', linewidth=linewidth)
    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    for (config, freq_data) in data.items():
        (mode, target_freq) = config
        k = freq_data.keys()
        v = freq_data.values()

        if mode == 'fastmode':
            linestyle = 'dotted'
            modename = 'Fast'
            color = 'dimgray'
        else:
            linestyle = 'solid'
            modename = 'Exact'
            color = 'black'

        if target_freq == 10:
            marker = 'o'
        elif target_freq == 50:
            marker = '^'
        elif target_freq == 70:
            marker = 's'
        else:
            print('Unknown target frequency')
            marker = '>'

        ax.plot(k, v, color=color,
                marker=marker,
                markersize=markersize,
                linestyle=linestyle,
                linewidth=linewidth,
                label=f'{modename}-{target_freq}MHz')

    ax.legend(bbox_to_anchor=(1.0, 1.0), fontsize=fontsize)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-11-qsfp-perf-sweep.png'))

def main():
    data = parse_input_csv_files()
    print(data)
    plot(data)

if __name__=="__main__":
    main()
