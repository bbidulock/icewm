#!/bin/bash

# a little script to create a distribution of the package

[ -f Makefile ] && make distclean
./autogen.sh
./configure.sh
ncores=$(nproc 2>/dev/null) || ncores=4 &&:
make -j$ncores distcheck

