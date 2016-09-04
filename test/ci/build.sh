#!/bin/sh

set -e
set -v

list_output_dir () {
        echo "====== Listing ${HOME}/test ======="
        ls -Rl ${HOME}/test
        echo "==================================="
}

if [ "${BUILD}" = "autotools" ]; then
        NOCONFIGURE=1 ./autogen.sh
        ./configure --disable-silent-rules --enable-werror CC=${CC} --prefix=${HOME}/test
        make
        make install
        list_output_dir
        example/sample-query
elif [ "${BUILD}" = "meson" ]; then
        mkdir build
        cd build
        CC=${CC} meson --prefix=${HOME}/test ..
        ninja
        ninja install
        list_output_dir
        ./sample-query
fi
