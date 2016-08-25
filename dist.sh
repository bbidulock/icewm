#!/bin/bash

# a little script to create a distribution of the package

[ -f Makefile ] && make distclean
./autogen.sh
./configure.sh
make distcheck

