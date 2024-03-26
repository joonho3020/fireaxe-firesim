#!/bin/bash

echo "Generate the correct config_hwdb.yaml"
./generate-config_hwdb.py --bitstream-dir=$(pwd)/../fireaxe-isca-ae-bitstreams
cp ../deploy/config_hwdb.yaml backup_config_hwdb.yaml
cp config_hwdb.yaml ../deploy

echo "Running qsfp perf sweeps"
./run-qsfp-width-sweep.sh

echo "Running FAME-5 perf sweeps"
./run-fame5-perf-sweep.sh

echo "Running core split experiment"
./run-tip-experiments.sh

echo "Running DDIO experiment"
./run-ddio-experiments.sh

echo "Running go GC experiment"
./ru-go-gc.sh


echo "-------------------------"
echo "run-ae-full.sh complete!!"
echo "-------------------------"
