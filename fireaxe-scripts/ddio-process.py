#!/usr/bin/env python3



import pandas as pd
import argparse
import os


parser = argparse.ArgumentParser(description="Arguments for processing DDIO outputs")
parser.add_argument("--sim-dir", type=str, required=True, help="Abs path to DDIO simulation dir")
parser.add_argument("--row-idx", type=str, required=True, help="Index value in the csv for these rows")
args = parser.parse_args()


with open(os.path.join(args.sim_dir, 'sim_slot_0/uartlog'), 'r') as uartlog:
    lines = uartlog.readlines()
    rd_bins = []
    wr_bins = []
    for line in lines:
        words = line.split()
        if len(words) == 2 and 'hist_rd' in words[0]:
            rd_bins.append(int(words[1]))
        elif len(words) == 2 and 'hist_wr' in words[0]:
            wr_bins.append(int(words[1]))

    row_idx = f'Rd-{args.row_idx}'
    print(row_idx, *rd_bins, sep=',')

    row_idx = f'Wr-{args.row_idx}'
    print(row_idx, *wr_bins, sep=',')
