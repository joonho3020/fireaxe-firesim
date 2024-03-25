#!/bin/bash

set -e

FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../
MARSHAL_DIR=$FIRESIM_BASEDIR/target-design/chipyard/software/firemarshal
FIRESIM_SIMULATION_DIR=$FIRESIM_BASEDIR/deploy/sim-dir
INTERMEDIATE_DIR=$FIREAXE_SCRIPT_DIR/qsfp-sweep-intermediate
RESULT_DIR=$FIREAXE_SCRIPT_DIR/perf-results

function copy_firesim_db() {
    echo "copy_firesim_db"
    cd $FIREAXE_SCRIPT_DIR
    sudo cp $FIREAXE_SCRIPT_DIR/firesim-db/firesim-db-2fpga-ae.json /opt/firesim-db.json
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
    TILES=$2
    OUT_CONFIG_FILE=$3
    SIM_DIR_PFX=$4
    FAST_OR_EXACT=$5
    if [ $FAST_OR_EXACT = "fastmode" ]
    then
        PARTITION_SEED=1
    else
        PARTITION_SEED=0
    fi
    ./generate-config-runtime.py \
        --sim-dir $FIRESIM_SIMULATION_DIR/$SIM_DIR_PFX-$TILES \
        --topology $TOPOLOGY \
        --tip-enable false \
        --partition-seed $PARTITION_SEED \
        --workload-name run-cycles.json \
        --out-config-file $OUT_CONFIG_FILE
    mv $OUT_CONFIG_FILE $INTERMEDIATE_DIR
    firesim infrasetup  -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim runworkload -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim kill -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
}

function firesim_run_all() {
    SIM_DIR_PFX=$1
    BITSTREAM_FREQ=$2
    FAST_OR_EXACT=$3

    echo "firesim_run_all $SIM_DIR_PFX $BITSTREAM_FREQ $FAST_OR_EXACT"

    TILELIST=("sixteen" "eight" "four" "two" "one")
    TOPO_PFX="qsfp_$FAST_OR_EXACT"

    for TILES in ${TILELIST[@]}; do
        echo "Run firesim infrasetup & firesim runworkload for $TILES"
        TOPOLOGY="${TOPO_PFX}_${TILES}_big_rocket_${BITSTREAM_FREQ}MHz_config"
        echo "Setting topology to $TOPOLOGY"
        firesim_infrasetup_runworkload $TOPOLOGY $TILES $FAST_OR_EXACT-tiles-$TILES-config_runtime.yaml $SIM_DIR_PFX $FAST_OR_EXACT
    done
}

function process_sweep_results() {
    SIM_DIR_PFX=$1
    BITSTREAM_FREQ=$2
    FAST_OR_EXACT=$3
    ./fame5-perf-sweep.py --top-sim-dir $FIRESIM_SIMULATION_DIR --sim-dir-pfx $SIM_DIR_PFX > $RESULT_DIR/qsfp-$FAST_OR_EXACT-$BITSTREAM_FREQ.csv
}


function run_for_frequency() {
    BITSTREAM_FREQ=$1
    FAST_OR_EXACT=$2
    SIM_DIR_PFX=qsfp-$FAST_OR_EXACT-$BITSTREAM_FREQ

    echo "run_for_frequency $BITSTREAM_FREQ $FAST_OR_EXACT"
    firesim_run_all $SIM_DIR_PFX $BITSTREAM_FREQ $FAST_OR_EXACT
    process_sweep_results $SIM_DIR_PFX $BITSTREAM_FREQ $FAST_OR_EXACT
}

function generate_plot() {
    echo "Generating QSFP perf sweep plot"
    cd $FIREAXE_SCRIPT_DIR
    ./plot-qsfp-width-sweep.py --input-dir=perf-results
}

function run_all() {
    copy_firesim_db
    build_and_install_workload

    run_for_frequency 10 "fastmode"
    run_for_frequency 50 "fastmode"
    run_for_frequency 70 "fastmode"

    run_for_frequency 10 "exactmode"
    run_for_frequency 50 "exactmode"
    generate_plot
}

time run_all | tee run-qsfp-width-sweep.log
