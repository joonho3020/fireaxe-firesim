#!/bin/bash

set -e

FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../
FIRESIM_SIMULATION_DIR=$FIRESIM_BASEDIR/deploy/sim-dir
INTERMEDIATE_DIR=$FIREAXE_SCRIPT_DIR/go-gc-intermediate

function copy_firesim_db() {
    echo "copy_firesim_db"
    cd $FIREAXE_SCRIPT_DIR
    sudo firesim-2fpga-config
}

function download_go() {
    cd $FIREAXE_SCRIPT_DIR
    echo "downloading go"
    wget https://go.dev/dl/go1.19.4.linux-amd64.tar.gz
    tar -xvzf go1.19.4.linux-amd64.tar.gz
    cd go/bin
    export PATH=$(pwd):$PATH
    cd ../../

    echo "Current go path"
    which go

    echo "Current go version"
    go version

    rm go1.19.4.linux-amd64.tar.gz
}

function build_and_install_workload() {
    cd $FIRESIM_BASEDIR/sw/firesim-software
    ./init-submodules.sh
    cd $FIREAXE_SCRIPT_DIR/go-gc-benchmark
    marshal build go-gc.json
    marshal install go-gc.json
    cd ../
    echo "BUILT & INSTALLED go-gc"
}

function firesim_infrasetup_runworkload() {
    OUT_CONFIG_FILE=go-gc-config_runtime.yaml
    ./generate-config-runtime.py \
        --sim-dir $FIRESIM_SIMULATION_DIR/go-gc \
        --topology fireaxe_four_gigaboom_config \
        --tip-enable false \
        --partition-seed 1 \
        --workload-name go-gc.json \
        --out-config-file $OUT_CONFIG_FILE
    mv $OUT_CONFIG_FILE $INTERMEDIATE_DIR
    firesim infrasetup  -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim runworkload -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    firesim kill -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
    rm $FIRESIM_BASEDIR/deploy/*.tar.gz
}

function copy_results() {
    cd $FIREAXE_SCRIPT_DIR
    RESULT_DIR_NAME=$(ls ../deploy/results-workload/ | grep "go-gc")
    RESULT_DIR=$FIRESIM_BASEDIR/deploy/results-workload/$RESULT_DIR_NAME
    cp $RESULT_DIR/go-gc0/GO_GC_RESULTS.out go-gc-results
}

function generate_plot() {
    echo "Generating GO GC experiment plot"
    cd $FIREAXE_SCRIPT_DIR
    ./plot-go-gc.py --input-dir=go-gc-results
}

function run_all() {
    copy_firesim_db
    download_go
    build_and_install_workload
    firesim_infrasetup_runworkload
    copy_results
    generate_plot
}

time run_all | tee run-go-gc.log
