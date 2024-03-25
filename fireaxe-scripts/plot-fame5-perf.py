#!/usr/bin/env python3

import os
import sys
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

parser = argparse.ArgumentParser(description='Draw FAME5-perf sweep result (Fig. 14)')
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
    'one'   : 1,
    'two'   : 2,
    'three' : 3,
    'four'  : 4,
    'five'  : 5,
    'six'   : 6
}

def parse_input_csv_files() -> Tuple[Dict[int, Dict[int, float]], int]:
    files = [f for f in os.listdir(args.input_dir) if re.match(r'fame5-perf-sweep-[0-9]+-[0-9]+\.csv', f)]

    data : Dict[int, Dict[int, float]] = dict()
    tile_freq = 0
    for file in files:
        (filename, _) = os.path.splitext(file)

        words = filename.split('-')
        subsys_freq = int(words[3])
        tile_freq   = int(words[4])

        subsys_freq_data : Dict[int, float] = dict()
        with open(os.path.join(args.input_dir, file), 'r') as f:
            first_line = True
            for line in csv.reader(f):
                if first_line:
                    first_line = False
                    continue

                num_tiles = str2int[line[0].split('-')[-1]]
                sim_freq_mhz = float(line[4]) / 1000
                subsys_freq_data[num_tiles] = sim_freq_mhz
        data[subsys_freq] = dict(sorted(subsys_freq_data.items()))
    return (data, tile_freq)

def plot(data: Dict[int, Dict[int, float]]):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Number of FAME-5 BOOM Tiles', fontsize=fontsize)
    ax.set_ylabel('Simulation Frequency (MHz)',  fontsize=fontsize)

    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    ax.set_ylim(0.3, 0.8)
    ax.set_yticks(ticks=[i * 0.1 for i in range(3, 9)])
    ax.set_xticks(ticks=[i + 0.5 for i in range(0, 6)], minor=True)

    ax.grid(axis="x", which='minor', linewidth=linewidth)
    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    for (subsys_freq, subsys_freq_data) in data.items():
        k = subsys_freq_data.keys()
        v = subsys_freq_data.values()

        if subsys_freq == 30:
            linestyle = 'dotted'
            marker = 'o'
        else:
            linestyle = 'solid'
            marker = 's'
        ax.plot(k, v, color='black', marker=marker, markersize=markersize, linestyle=linestyle, linewidth=linewidth, label=f'Sim {subsys_freq}MHz-15MHz')

    ax.legend(bbox_to_anchor=(1.0, 1.0), fontsize=fontsize)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-14-fame5-perf-sweep.png'))

def main():
    (data, tile_freq) = parse_input_csv_files()
    print(data)
    plot(data)

if __name__=="__main__":
    main()
