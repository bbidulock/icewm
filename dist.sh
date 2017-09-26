#!/bin/bash

# a little script to create a distribution of the package

[ -f Makefile ] && make distclean
./autogen.sh
./configure.sh
make -j$(($(nproc 2>/dev/null||echo 4)<<1)) distcheck

