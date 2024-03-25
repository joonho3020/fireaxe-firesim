#!/bin/bash

set -e

FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../
FIRESIM_SIMULATION_DIR=$FIRESIM_BASEDIR/deploy/sim-dir
INTERMEDIATE_DIR=$FIREAXE_SCRIPT_DIR/ddio-intermediate
OUTPUT_DIR=$FIREAXE_SCRIPT_DIR/ddio-results

CYDIR=$FIRESIM_BASEDIR/target-design/chipyard

ICENIC_DIR=$CYDIR/generators/icenet
ICENIC_SW=$ICENIC_DIR/software

function checkout_firesim_ddio() {
    cd $FIRESIM_BASEDIR
    git checkout ae-ddio
    cd target-design/chipyard
    git checkout ae-ddio
    ./scripts/init-submodules-no-riscv-tools-nolog.sh -f
    cd $FIREAXE_SCRIPT_DIR
}

function checkout_firesim_ae_main() {
    cd $FIRESIM_BASEDIR
    git checkout ae-main
    cd target-design/chipyard
    git checkout ae-main
    ./scripts/init-submodules-no-riscv-tools-nolog.sh -f
    cd $FIREAXE_SCRIPT_DIR
}

function compile_network_sw() {
  cd $ICENIC_SW
  COMMON_COMPILE_COMMANDS="riscv64-unknown-elf-gcc -specs=htif_nano.specs -Wall"
  OBJDUMP_COMPILE_COMMANDS="riscv64-unknown-elf-objdump -D"

  $COMMON_COMPILE_COMMANDS -fno-common -fno-builtin-printf --no-builtin-rules -DTX_CORES=$1 -o traffic-gen.o -c traffic-gen.c
  $COMMON_COMPILE_COMMANDS -static traffic-gen.o -o traffic-gen.riscv
  $OBJDUMP_COMPILE_COMMANDS traffic-gen.riscv > traffic-gen.dump

  $COMMON_COMPILE_COMMANDS -fno-common -fno-builtin-printf --no-builtin-rules -DRUN_CORES=$2 -o traffic-recv.o -c traffic-recv.c
  $COMMON_COMPILE_COMMANDS -static traffic-recv.o -o traffic-recv.riscv
  $OBJDUMP_COMPILE_COMMANDS traffic-recv.riscv > traffic-recv.dump

  cp traffic-*.riscv ddio-test
  cd $FIREAXE_SCRIPT_DIR
}

function firesim_infrasetup_runworkload() {
  cd $FIREAXE_SCRIPT_DIR
  TOPOLOGY=$1
  RX_CORES=$2
  PARTITION_SEED=$3
  OUT_CONFIG_FILE=$TOPOLOGY-$RX_CORES-config_runtime.yaml

  compile_network_sw 12 $RX_CORES

  ./generate-config-runtime.py \
    --sim-dir $FIRESIM_SIMULATION_DIR/$TOPOLOGY-$RX_CORES \
    --topology $TOPOLOGY \
    --partition-seed $PARTITION_SEED \
    --workload-name ddio-test.json \
    --run-farm-hosts-to-use "four_fpgas_spec" \
    --out-config-file $OUT_CONFIG_FILE

  mv $OUT_CONFIG_FILE $INTERMEDIATE_DIR

  echo "Start firesim simulations for $TOPOLOGY $RX_CORES"
  firesim infrasetup  -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
  firesim runworkload -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
  firesim kill -c $INTERMEDIATE_DIR/$OUT_CONFIG_FILE
}

function run_xbar_config() {
  cd $FIREAXE_SCRIPT_DIR

  CORES=(1 2 4 6 8 10 12)
  TOPOLOGY=fireaxe_128kB_dodeca_boom_xbar_config
  for RX_CORES in ${CORES[@]}; do
    firesim_infrasetup_runworkload $TOPOLOGY $RX_CORES 1
  done

  echo "[*] Start post processing $TOPOLOGY"
  RESULTS_FILE=$OUTPUT_DIR/$TOPOLOGY-DDIO-HITRATES.csv
  if [ -f $RESULTS_FILE ]; then
    rm -f $RESULTS_FILE
  fi
  touch $RESULTS_FILE

  for RX_CORES in ${CORES[@]}; do
    SIM_DIR=$FIRESIM_SIMULATION_DIR/$TOPOLOGY-$RX_CORES
    ./ddio-process.py --sim-dir $SIM_DIR --row-idx $RX_CORES >> $RESULTS_FILE
  done
}

function run_noc_config() {
  cd $FIREAXE_SCRIPT_DIR

  CORES=(1 2 4 6 8 10 12)
  TOPOLOGY=fireaxe_128kB_dodeca_boom_ring_noc_config
  for RX_CORES in ${CORES[@]}; do
    firesim_infrasetup_runworkload $TOPOLOGY $RX_CORES 0
  done

  echo "[*] Start post processing $TOPOLOGY"
  RESULTS_FILE=$OUTPUT_DIR/$TOPOLOGY-DDIO-HITRATES.csv
  if [ -f $RESULTS_FILE ]; then
    rm -f $RESULTS_FILE
  fi
  touch $RESULTS_FILE

  for RX_CORES in ${CORES[@]}; do
    SIM_DIR=$FIRESIM_SIMULATION_DIR/$TOPOLOGY-$RX_CORES
    ./ddio-process.py --sim-dir $SIM_DIR --row-idx $RX_CORES >> $RESULTS_FILE
  done
}

checkout_firesim_ddio
cd /opt
sudo cp firesim-db-one-cherry-one-mono.json firesim-db.json
cd -
run_xbar_config
checkout_firesim_ae_main

# Need to change the inter-FPGA connection
# cd /opt
# sudo cp firesim-db-one-ring-one-mono.json firesim-db.json
# cd -
# run_noc_config
