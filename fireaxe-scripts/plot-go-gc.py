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

parser = argparse.ArgumentParser(description='Draw golang garbage collection case study plot (Fig. 10)')
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

            # numcores  percentail      gomaxprocs  perf
Data = Dict[Tuple[str, int], Dict[int, float]]

def parse_input_csv_files() -> Data:
    cur_gomaxprocs = 0
    cur_numcores = 0
    data: Data = dict()

    file = os.path.join(args.input_dir, 'GO_GC_RESULTS.out')
    with open(file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            words = line.split()
            if len(words) == 2 and words[0] == 'GOMAXPROCS':
                cur_gomaxprocs = int(words[1])
            elif len(words) == 5 and words[0] == 'Pin':
                cur_numcores = words[3]
            elif len(words) == 2:
                percentile = int(words[0][:2])
                if percentile == 50:
                    continue
                else:
                    unit = words[1][-2:]
                    latency = float(words[1][:-2])
                    if unit != 'ms':
                        latency = latency / 1000
                    if (cur_numcores, percentile) not in data.keys():
                        data[(cur_numcores, percentile)] = dict()
                    data[(cur_numcores, percentile)][cur_gomaxprocs] = latency
    return data

def plot(data: Data):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    print(fig.axes)
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('GOMAXPROCS',        fontsize=fontsize)
    ax.set_ylabel('Tail Latency (ms)', fontsize=fontsize)

    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    ax.set_ylim(0, 180)
    ax.set_yticks(ticks=[i * 40 for i in range(5)])

    ax.set_xticks(ticks=[i for i in range(4)])
    ax.set_xticks(ticks=[i + 0.5 for i in range(0, 3)], minor=True)

    ax.grid(axis="x", which='minor', linewidth=linewidth)
    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    for (config, freq_data) in data.items():
        (cores, percentile) = config
        k = freq_data.keys()
        v = freq_data.values()

        if cores == 'GOMAXPROCS':
            color = 'black'
            cores = cores + ' cores'
        else:
            color = 'dimgray'
            cores = cores + ' core'

        if percentile == 99:
            marker = 'o'
            linestyle = 'solid'
        else:
            marker = '^'
            linestyle = 'dotted'

        ax.plot(k, v, color=color,
                marker=marker,
                markersize=markersize,
                linestyle=linestyle,
                linewidth=linewidth,
                label=f'{cores} {percentile}th percentile')

    ax.legend(bbox_to_anchor=(1.0, 1.0), fontsize=fontsize + 6)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-10-go-gc.png'))

def main():
    data = parse_input_csv_files()
    print(data)
    plot(data)

if __name__=="__main__":
    main()
