#!/bin/bash

set -e

FIREAXE_SCRIPT_DIR=$(pwd)
FIRESIM_BASEDIR=$FIREAXE_SCRIPT_DIR/../

cd $FIRESIM_BASEDIR
git checkout ae-main
cd target-design/chipyard
git checkout ae-main
./scripts/init-submodules-no-riscv-tools-nolog.sh -f
cd $FIREAXE_SCRIPT_DIR
