#!/usr/bin/env python3


import argparse
import os


parser = argparse.ArgumentParser(description="Arguments for processing FAME-5 performance sweep outputs")
parser.add_argument("--top-sim-dir", type=str, required=True, help="Abs path to top simulation directory")
parser.add_argument("--sim-dir-pfx", type=str, required=True, help="Prefix for FAME-5 simulation directories")
args = parser.parse_args()

def get_results_for_thread(sim_dir):
    results = {}
    with open(os.path.join(sim_dir, 'sim_slot_0', 'uartlog'), 'r') as uartlog:
        lines = uartlog.readlines()
        for line in lines:
            words = line.split()
            if len(words) >= 4 and words[0] == 'Wallclock':
                results['wallclock'] = float(words[3])
            elif len(words) >=4 and words[0] == 'Host' and words[1] == 'Frequency:':
                results['hostfreq'] = float(words[2])
            elif len(words) >= 4 and words[0] == 'Target' and words[1] == 'Cycles':
                results['targetcycles'] = int(words[3])
            elif len(words) >= 5 and words[0] == 'Effective' and words[1] == 'Target' and words[2] == 'Frequency:':
                if words[4] == 'KHz':
                    results['simfreq'] = float(words[3])
                else:
                    assert(words[4] == 'MHz')
                    results['simfreq'] = float(words[3]) * 1000
            elif len(words) >= 2 and words[0] == 'FMR:':
                results['fmr'] = float(words[1])
    return results

def get_all_results():
    print(f'Name,wallclock(s),hostfreq(MHz),targetcycles(#),sim-freq(kHz),FMR')
    for sim_dir in os.listdir(args.top_sim_dir):
        if os.path.isdir(os.path.join(args.top_sim_dir, sim_dir)) and sim_dir.startswith(args.sim_dir_pfx):
            results = get_results_for_thread(os.path.join(args.top_sim_dir, sim_dir))
            wc = results['wallclock']
            hf = results['hostfreq']
            tc = results['targetcycles']
            sf = results['simfreq']
            fmr = results['fmr']
            print(f'{sim_dir},{wc},{hf},{tc},{sf},{fmr}')

def main():
    get_all_results()

if __name__=="__main__":
    main()
