#!/bin/bash

# a little script to create a distribution of the package

[[ $1 =~ ^-j ]] && jobs=$1 ||
    jobs=-j$(($(nproc 2>/dev/null||echo 4)<<1))

[ -f Makefile ] && make distclean
./autogen.sh
./configure.sh
make "$jobs" distcheck RELEASE.txt

