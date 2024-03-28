#!/bin/bash

echo "Generate the correct config_hwdb.yaml"
./generate-config_hwdb.py --bitstream-dir=$(pwd)/../fireaxe-isca-ae-bitstreams
cp ../deploy/config_hwdb.yaml backup_config_hwdb.yaml
cp config_hwdb.yaml ../deploy


SKIP_LIST=()

while [ "$1" != "" ];
do
    case $1 in
        --skip | -s)
            shift
            SKIP_LIST+=(${1}) ;;
        * )
            error "invalid option $1"
            usage 1 ;;
    esac
    shift
done

# return true if the arg is not found in the SKIP_LIST
run_step() {
    local value=$1
    [[ ! " ${SKIP_LIST[*]} " =~ " ${value} " ]]
}

if run_step "1"; then
    echo "Running qsfp perf sweeps"
    ./run-qsfp-width-sweep.sh
fi

if run_step "2"; then
    echo "Running FAME-5 perf sweeps"
    ./run-fame5-perf-sweep.sh
fi

if run_step "3"; then
    echo "Running DDIO experiment"
    ./run-ddio-experiments.sh
    ./checkout-ae-main.sh
fi

if run_step "4"; then
    echo "Running go GC experiment"
    ./run-go-gc.sh
fi

if run_step "5"; then
    echo "Running core split experiment"
    ./run-tip-experiments.sh
    ./checkout-ae-main.sh
fi

echo "-------------------------"
echo "run-ae-full.sh complete!!"
echo "-------------------------"
