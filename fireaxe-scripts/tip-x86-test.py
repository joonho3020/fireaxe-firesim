#!/usr/bin/env python3

import os
import sys
import argparse
from tqdm import tqdm



parser = argparse.ArgumentParser(description="Arguments for processing X86 IPC")
parser.add_argument("--binary-dir", type=str, required=True, help="Binary directory")
parser.add_argument("--repeat", type=int, default=2, help="Number of times to repeat measurement")
args = parser.parse_args()


def bash(cmd):
    fail = os.system(cmd)
    if fail:
        print(f'[*] failed to execute {cmd}')


def parse_output(dump):
    insts = 0
    cycles = 0
    wallclock = 0.0
    with open(dump, 'r') as f:
        lines = f.readlines()
        for line in lines:
            words = line.split()
            if len(words) >= 2 and words[1] == 'instructions':
                insts = int(words[0].replace(',', ''))
            if len(words) >= 2 and words[1] == 'cycles':
                cycles = int(words[0].replace(',', ''))
            if len(words) == 3 and words[0] == 'WallClockTime:':
                wallclock = float(words[1])
    return (cycles, insts, wallclock)


def run(binary):
    bash(f"sudo perf stat -d ./{binary} &> /tmp/PERF")
    return parse_output('/tmp/PERF')

def get_ipc(binary, repeat):
    cycles = 0
    insts =  0
    wall_clock = 0.0
    for _ in tqdm(range(repeat)):
        (c, i, wc) = run(binary)
        cycles += c
        insts += i
        wall_clock += wc
    return (float(insts / cycles), (wall_clock / repeat))

def main():
    binary_dir = args.binary_dir
    os.chdir(binary_dir)
    print(os.getcwd())
    binaries = [f for f in os.listdir() if os.path.isfile(f)]
    print('name,xeon-ipc,wallclocktime')
    for binary in binaries:
        (ipc, wct) = get_ipc(binary, args.repeat)
        print(f"{binary},{ipc},{wct}")

if __name__ == '__main__':
    main()
