#!/bin/bash

rm -f cscope.*
./autogen.sh
./configure.sh
make V=0 clean cscope all
