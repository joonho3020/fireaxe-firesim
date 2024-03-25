#!/bin/bash

set -e


FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../
MARSHAL_DIR=$FIRESIM_BASEDIR/target-design/chipyard/software/firemarshal
FIRESIM_SIMULATION_DIR=$FIRESIM_BASEDIR/deploy/sim-dir
INTERMEDIATE_DIR=$FIREAXE_SCRIPT_DIR/fpga-cnt-sweep-intermediate
RESULT_DIR=$FIREAXE_SCRIPT_DIR/perf-results


function build_and_install_workload() {
    cd $FIREAXE_SCRIPT_DIR/perf-sweep-workload
    make clean && make
    cd $MARSHAL_DIR
    ./marshal install $FIREAXE_SCRIPT_DIR/perf-sweep-workload/run-cycles.json
    cd $FIREAXE_SCRIPT_DIR
    echo "BUILT & INSTALLED run-cycles"
}


function firesim_infrasetup_runworkload() {
    TOPOLOGY=$1
    FPGA_CNT=$2
    FREQUENCY=$3
    OUT_CONFIG_FILE=$4
    SIM_DIR_PFX=$5

    ./generate-config-runtime.py \
        --sim-dir $FIRESIM_SIMULATION_DIR/$SIM_DIR_PFX-$FPGA_CNT-$FREQUENCY \
        --topology $TOPOLOGY \
        --tip-enable false \
        --partition-seed 0 \
        --workload-name run-cycles.json \
        --out-config-file $OUT_CONFIG_FILE
    mv $OUT_CONFIG_FILE $INTERMEDIATE_DIR
    firesim infrasetup  -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim runworkload -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim kill        -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
}

function firesim_run_all() {
# FREQS=("10" "30" "50" "70")
# FREQS=("10")
# FREQS=("30")
# FREQS=("50")
    FREQS=("70")
    FPGA_CNT=$1

# for FREQUENCY in ${FREQS[@]}; do
# echo "Run firesim infrasetup & firesim runworkload for $FPGA_CNT FPGAs with $FREQUENCY"
# TOPOLOGY="qsfp_${FPGA_CNT}fpga_${FREQUENCY}MHz"
# PREFIX="fpga-cnt-${FPGA_CNT}-${FREQUENCY}"
# OUT_CONFIG_FILE="${PREFIX}-config_runtime.yaml"
# firesim_infrasetup_runworkload $TOPOLOGY $FPGA_CNT $FREQUENCY $OUT_CONFIG_FILE $PREFIX
# done
    SIM_DIR_PREFIX="fpga-cnt-${FPGA_CNT}"
    ./fame5-perf-sweep.py --top-sim-dir $FIRESIM_SIMULATION_DIR --sim-dir-pfx $SIM_DIR_PREFIX > $RESULT_DIR/fpga-cnt-perf-sweep-$FPGA_CNT.csv
}

build_and_install_workload
# firesim_run_all 3
# firesim_run_all 4
# firesim_run_all 5
firesim_run_all 6
