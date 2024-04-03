#!/bin/bash

set -e

FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../
MARSHAL_DIR=$FIRESIM_BASEDIR/target-design/chipyard/software/firemarshal
FIRESIM_SIMULATION_DIR=$FIRESIM_BASEDIR/deploy/sim-dir
INTERMEDIATE_DIR=$FIREAXE_SCRIPT_DIR/fame5-sweep-intermediate
RESULT_DIR=$FIREAXE_SCRIPT_DIR/perf-results

function copy_firesim_db() {
    echo "copy_firesim_db"
    cd $FIREAXE_SCRIPT_DIR
    sudo firesim-2fpga-config
}

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
    THREADS=$2
    OUT_CONFIG_FILE=$3
    SIM_DIR_PFX=$4
    ./generate-config-runtime.py \
        --sim-dir $FIRESIM_SIMULATION_DIR/$SIM_DIR_PFX-$THREADS \
        --topology $TOPOLOGY \
        --tip-enable false \
        --partition-seed 1 \
        --workload-name run-cycles.json \
        --out-config-file $OUT_CONFIG_FILE
    mv $OUT_CONFIG_FILE $INTERMEDIATE_DIR
    firesim infrasetup  -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim runworkload -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim kill        -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
}

function firesim_run_all() {
    SIM_DIR_PFX=$1
    SUBSYS_BITSTREAM_FREQ=$2

    THREADLIST=("six" "five" "four" "three" "two" "one")
    TOPO_PFX="qsfp_fame5_fastmode"
    TOPO_SFX="large_boom_${SUBSYS_BITSTREAM_FREQ}MHz_config"

    for THREADS in ${THREADLIST[@]}; do
        echo "Run firesim infrasetup & firesim runworkload for $THREADS FAME-5 threads"
        TOPOLOGY="${TOPO_PFX}_${THREADS}_${TOPO_SFX}"
        echo "Setting topology to $TOPOLOGY"
        firesim_infrasetup_runworkload $TOPOLOGY $THREADS fame5-threadcnt-$THREADS-config_runtime.yaml $SIM_DIR_PFX
    done
}

function process_sweep_results() {
    SIM_DIR_PFX=$1
    SIM_FREQS=$2
    ./fame5-perf-sweep.py --top-sim-dir $FIRESIM_SIMULATION_DIR --sim-dir-pfx $SIM_DIR_PFX > $RESULT_DIR/fame5-perf-sweep-$SIM_FREQS.csv
}

function run_for_frequency() {
    SUBSYS_BITSTREAM_FREQ=$1
    TILE_BITSTREAM_FREQ=$2
    SIM_FREQS=$SUBSYS_BITSTREAM_FREQ-$TILE_BITSTREAM_FREQ
    SIM_DIR_PFX=fame5-sweep-threadcnt-$SIM_FREQS

    firesim_run_all  $SIM_DIR_PFX $SUBSYS_BITSTREAM_FREQ
    process_sweep_results $SIM_DIR_PFX $SIM_FREQS
}

function generate_plot() {
    echo "Generating FAME5 perf sweep plot"
    cd $FIREAXE_SCRIPT_DIR
    ./plot-fame5-perf.py --input-dir=perf-results
}

function run_all() {
    copy_firesim_db
    build_and_install_workload
    run_for_frequency 30 15
    run_for_frequency 20 15
    generate_plot
}

time run_all | tee run-fame5-perf-sweep.log
