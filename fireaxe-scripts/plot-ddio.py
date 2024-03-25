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

parser = argparse.ArgumentParser(description='Draw ddio case study plot (Fig. 9)')
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

            # bus-topo  rd/wr    ncores  cycles
Data = Dict[Tuple[str, str], Dict[int, float]]

def parse_input_csv_files() -> Data:
    files = [f for f in os.listdir(args.input_dir) if re.match(r'fireaxe.+\.csv', f)]
    print(files)

    data: Data = dict()
    for file in files:
        (filename, _) = os.path.splitext(file)

        words = filename.split('-')
        if 'noc' in words[0]:
            bus_topo = 'Ring'
        else:
            bus_topo = 'Xbar'

        data[(bus_topo, 'Rd')] = dict()
        data[(bus_topo, 'Wr')] = dict()
        with open(os.path.join(args.input_dir, file), 'r') as f:
            for line in csv.reader(f):
                [rw, ncores] = line[0].split('-')
                data[(bus_topo, rw)][int(ncores)] = float(line[1])
    return data

def plot(data: Data):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    print(fig.axes)
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Number of Cores Forwarding Packets', fontsize=fontsize)
    ax.set_ylabel('Avg. Req to Resp Lat (cycles)',      fontsize=fontsize)

    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    ax.set_ylim(60, 220)
    ax.set_yticks(ticks=[i * 20 for i in range(3, 12)])

    ax.set_xticks(ticks=[i for i in range(1, 8)])
    ax.set_xticks(ticks=[i + 0.5 for i in range(0, 7)], minor=True)
    ax.set_xticklabels([1, 2, 4, 6, 8, 10, 12])

    ax.grid(axis="x", which='minor', linewidth=linewidth)
    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    for (config, cycle_data) in data.items():
        (topo, rw) = config
        k = [(ncores//2 + 1) for ncores in cycle_data.keys()]
        v = cycle_data.values()

        if topo == 'Ring':
            color = 'gray'
            marker = '^'
        else:
            color = 'black'
            marker = 'o'

        if rw == 'Rd':
            linestyle = 'solid'
        else:
            linestyle = 'dotted'

        ax.plot(k, v, color=color,
                marker=marker,
                markersize=markersize,
                linestyle=linestyle,
                linewidth=linewidth,
                label=f'{topo} {rw} Lat')

    ax.legend(bbox_to_anchor=(1.0, 1.0), fontsize=fontsize + 6)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-9-ddio.png'))

def main():
    data = parse_input_csv_files()
    print(data)
    plot(data)

if __name__=="__main__":
    main()
