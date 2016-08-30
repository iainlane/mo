#!/bin/sh

set -e
set -x

NOCONFIGURE=1 ./autogen.sh
./configure --disable-silent-rules --enable-werror CC=${CC} --prefix=${HOME}/temp
make
make install
example/sample-query
