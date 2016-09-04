#!/bin/sh

set -e
set -v

list_output_dir () {
        echo "====== Listing ${HOME}/test ======="
        ls -Rl ${HOME}/test
        echo "==================================="
}

mkdir build
cd build
CC=${CC} meson --prefix=${HOME}/test ..
ninja
ninja install
list_output_dir
./sample-query
