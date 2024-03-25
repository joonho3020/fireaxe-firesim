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
import pandas as pd
import re
from typing import Dict, List, Tuple, Set


matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

parser = argparse.ArgumentParser(description='Draw golang garbage collection case study plot (Fig. 7, 8)')
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

                       #   bm-list       coretype wallclocktime-list
WallClockTimeData = Tuple[List[str], Dict[str, List[float]]]

def geomean(xs):
        return math.exp(math.fsum(math.log(x) for x in xs) / len(xs))

def parse_wallclocktime_csv_files() -> WallClockTimeData:
    files = [f for f in os.listdir(args.input_dir) if re.match(r'TIP-IPC-.+\.csv', f)]
    print(files)

    data: Dict[str, List[float]] = dict()
    benchmarks: Set[str] = set()
    for file in files:
        (filename, _) = os.path.splitext(file)
        words = filename.split('-')
        coretype = words[2]

        core_wallclocktime : Dict[str, float] = dict()
        with open(os.path.join(args.input_dir, file), 'r') as f:
            first_line = True
            for line in csv.reader(f):
                if first_line:
                    first_line = False
                    continue

                if coretype == 'XEON':
                    bm = line[0]
                    wallclocktime = float(line[2]) * (10**6)
                else:
                    bm = line[0][2:]
                    wallclocktime = int(line[2]) / (3.4 * (10**3))

                benchmarks.add(bm)
                core_wallclocktime[bm] = wallclocktime

        core_wallclocktime = dict(sorted(core_wallclocktime.items()))
        wallclocktime_geomean = geomean(core_wallclocktime.values())
        wallclocktime_bms = list(core_wallclocktime.values())
        wallclocktime_bms.append(wallclocktime_geomean)

        data[coretype] = wallclocktime_bms

    benchmarks_list = list(sorted(benchmarks))
    benchmarks_list.append('GeoMean')
    return (benchmarks_list, data)

def plot_wallclocktime_fig(data: WallClockTimeData):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    ax = fig.add_subplot(1, 1, 1)

    ax.set_ylabel('Runtime (us)',  fontsize=fontsize)
    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    ax.set_ylim(0, 900)
    ax.set_yticks(ticks=[i * 100 for i in range(9)])

    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    width = 0.25
    bar_width = width - 0.05
    multiplier = 0
    (benchmarks, core_perf) = data
    x = np.arange(len(benchmarks))

    corelist = ['large', 'gc', 'XEON']
    colormap = {
        'large' : 'dimgray',
        'gc' : 'black',
        'XEON' : 'lightgray'
    }
    for core in corelist:
        perf = core_perf[core]
        offset = width * multiplier
        rects = ax.bar(x + offset, perf,
                       width=bar_width,
                       label=core,
                       color=colormap[core],
                       edgecolor='black')
        multiplier += 1

    ax.set_xticks(ticks=x + width)
    ax.set_xticklabels(labels=benchmarks, rotation=45, ha='right')

    ax.legend(bbox_to_anchor=(1.0, 1.0), fontsize=fontsize)
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-7-wallclocktime.png'))

#                    commit / ld stall / st stall / alu stall / frontend / misc
# sglib-combined
# matmult-int
# nettle-aes
# huffbench
# nbody

#             coretype table
CPIData = Dict[str, Dict[str, Dict[str, float]]]

def parse_tip_pipeline_files() -> Tuple[List[str], CPIData]:
    files = [f for f in os.listdir(args.input_dir) if re.match(r'TIP-OUTPUT-.+\.csv', f)]
    print(files)

    cpi_data: CPIData = dict()
    pre_sorted_cpi : CPIData = dict()
    bm_interest = ['sglib-combined', 'matmult-int', 'nettle-aes', 'huffbench', 'nbody']

    for file in files:
        (filename, _) = os.path.splitext(file)
        words = filename.split('-')
        coretype = words[3]
        pre_sorted_cpi[coretype] = dict()

        with open(os.path.join(args.input_dir, file), 'r') as f:
            first_line = True
            for line in csv.reader(f):
                if first_line:
                    first_line = False
                    continue

                for bm in bm_interest:
                    if bm in line[0]:
                        pre_sorted_cpi[coretype][bm] = dict()
                        pre_sorted_cpi[coretype][bm]['commit']    = float(line[2])
                        pre_sorted_cpi[coretype][bm]['ld_stall']  = float(line[3])
                        pre_sorted_cpi[coretype][bm]['st_stall']  = float(line[4])
                        pre_sorted_cpi[coretype][bm]['alu_stall'] = float(line[5])
                        pre_sorted_cpi[coretype][bm]['frontend']  = float(line[6])
                        pre_sorted_cpi[coretype][bm]['misc']      = float(line[7]) + float(line[8])
    return (bm_interest, pre_sorted_cpi)


def plot_cpi_stack(data: Tuple[List[str], CPIData]):
    fig = plt.figure(figsize=figsize, dpi=args.dpi, clear=True)
    ax = fig.add_subplot(1, 1, 1)

    ax.tick_params(axis="x", which="major", direction="inout", labelsize=fontsize)
    ax.tick_params(axis="y", which="major", direction="inout", labelsize=fontsize)

    yticks = [i * 10 for i in range(11)]
    ytick_labels = [str(p) + '%' for p in yticks]

    ax.set_ylim(0, 100)
    ax.set_yticks(ticks=yticks, labels=ytick_labels)
    ax.grid(axis="y", linewidth=linewidth)
    ax.set_axisbelow(True)

    width = 0.25
    bar_width = width + 0.2
    multiplier = 0

    (benchmarks, cpi_stack) = data
    coretypes = cpi_stack.keys()

    xticks = np.arange(len(benchmarks) * len(coretypes))
    xtick_labels_minor = list()

    for bm in benchmarks:
        xtick_labels_minor.append(f'Large       GC40\n\n{bm}')

    num_bms = len(benchmarks)
    ax.set_xticks(ticks=np.arange(num_bms-1)*2 + 1.5, labels='' * (num_bms - 1))
    ax.set_xticks(ticks=np.arange(num_bms  )*2 + 0.5, labels=xtick_labels_minor, minor=True)
    ax.tick_params(axis='x', which='minor', labelsize=30)
    ax.tick_params(axis='x', which='major', direction='out', length=100)

    colormap = {
        'commit' : 'dimgray',
        'ld_stall' : 'white',
        'st_stall' : 'whitesmoke',
        'alu_stall' : 'black',
        'frontend' : 'white',
        'misc' : 'darkgray'
    }

    hatchmap = {
        'commit' : None,
        'ld_stall' : '||',
        'st_stall' : None,
        'alu_stall' : None,
        'frontend' : '//',
        'misc' : None
    }

    bar_bottoms: Dict[str, List[float]] = dict()
    bar_heights: Dict[str, List[float]] = dict()
    for bm in benchmarks:
        for core in coretypes:
            cpi_stack_info = cpi_stack[core][bm]
            print(cpi_stack_info)
            offset = width * multiplier
            cur_height = 0.0
            for (stall_type, ratio) in cpi_stack_info.items():
                height = ratio * 100
                if stall_type not in bar_bottoms:
                    bar_bottoms[stall_type] = list()
                if stall_type not in bar_heights:
                    bar_heights[stall_type] = list()
                bar_bottoms[stall_type].append(cur_height)
                bar_heights[stall_type].append(height)
                cur_height += height
            multiplier += 1

    for stall_type in bar_bottoms.keys():
        ax.bar(xticks,
            height=bar_heights[stall_type],
            width=bar_width,
            bottom=bar_bottoms[stall_type],
            color=colormap[stall_type],
            edgecolor='black',
            label=stall_type,
            hatch=hatchmap[stall_type])


    ax.legend(loc='lower center', fontsize=fontsize, ncols=len(bar_bottoms.keys()))
    plt.tight_layout()
    plt.savefig(os.path.join(args.output_dir, 'figure-8-cpistack.png'))

def main():
    wallclocktime_data = parse_wallclocktime_csv_files()
    plot_wallclocktime_fig(wallclocktime_data)

    (bm, cpi) = parse_tip_pipeline_files()
    plot_cpi_stack((bm, cpi))

if __name__=="__main__":
    main()
