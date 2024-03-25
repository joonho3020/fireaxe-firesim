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
        if len(words) == 4 and 'rd' in words[0] and 'wr' in words[2]:
            rd_bins.append(int(words[1]))
            wr_bins.append(int(words[3]))

    print(f'Rd-{args.row_idx},{sum(rd_bins)/len(rd_bins)}')
    print(f'Wr-{args.row_idx},{sum(wr_bins)/len(wr_bins)}')
