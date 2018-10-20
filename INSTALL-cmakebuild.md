Building IceWM with CMake
=========================

There is an experimental build system that you can use to compile
and install icewm. It's constructed around CMake and needs a
sufficiently modern CMake version for the processing
(at least version 2.6, which was released in May 2008).

Installation instructions:
--------------------------

 * for system requirements, see the [INSTALL](INSTALL) file.
 * run: `mkdir build ; cd build ; cmake ..`
 * check the output.
   To change variables, rerun the command above and add:
     `-DVARIABLE=replacement_value`
   To change the installation prefix to /opt/icewm run:
     `cmake -DCMAKE_INSTALL_PREFIX:PATH=/opt/icewm ..`
   If some values are sticky or some checks are not rerun
   (although you might have installed the reported missing packages),
   delete the file CMakeCache.txt and try again.
 * run: `make -j4`
   Adjust the last argument as needed to the number of CPUs,
   `-j$(nproc)` is a possible way on GNU.
 * run: `make install`

Developer/Integrator notes:
---------------------------

 * you can see more internals and documented variables with "cmake -LAH"
 * CMAKE_CXX_FLAGS are appended to default compiler options. To override
   completely, see CXXFLAGS_COMMON in src/CMakeLists.txt
 * use cmake -DCMAKE_INSTALL_PREFIX=... (or related variables, see
   CMakeLists.xt) to specify the target file space (default is /usr/local)
 * make install DESTDIR=/tmp/elsewhere copies to another target location
 * Potential tuning options for smaller and/or smaller binaries, build
   analysis, ...
   -DEXTRA_LIBS="-lsupc++"
   -DENABLE_LTO=ON
   -DEXTRA_MSGMERGE=--verbose -DEXTRA_MSGFMT=--verbose
 * There is also a configuration example for debug builds in rebuild.sh,
   see there fore details

[ vim: set ft=markdown: ]: #
