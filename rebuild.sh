#!/bin/bash

if test -z "$DBGCM"; then

rm -f cscope.*
./autogen.sh
./configure.sh
make clean
make V=0 cscope
cscope -b
make V=0 clean all README

else
# cmake cheat sheet... with Debian-style configuration
mkdir -p builddir-debug || rm -rf builddir-debug/CMake*
cd builddir-debug && cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DICEHELPIDX=/usr/share/doc/icewm-common/html/icewm.html -DCFGDIR=/etc/X11/icewm -DVERSION=10.9.8-debug -DDOCDIR=/usr/share/doc/icewm-common -DCMAKE_VERBOSE_MAKEFILE=ON -DCONFIG_XRANDR=on -DCONFIG_GUIEVENTS=on && make -j$(nproc || echo 3)
fi
