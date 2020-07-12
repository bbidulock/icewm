Building IceWM with CMake
=========================

The CMake build system for IceWM is an alternative
to the traditional configure build system.
The minimum supported CMake version is 2.6 (May 2008).

Installation instructions:
--------------------------

 * for system requirements, see the [INSTALL](INSTALL) file.
 * run: `mkdir build ; cd build ; cmake ..`
 * check the output.
   To change variables, rerun the command above and add:
     `-DVARIABLE=value`.
   To change the installation prefix to /opt/icewm run:
     `cmake -DCMAKE_INSTALL_PREFIX:PATH=/opt/icewm ..`.
   If some values are sticky or some checks are not rerun
   (although you might have installed the reported missing packages),
   delete the file CMakeCache.txt and try again.
 * run: `make -j4`
 * run: `make install`

Developer/Integrator notes:
---------------------------

 * you can see more internals and documented variables with "cmake -LAH"
 * CMAKE\_CXX\_FLAGS are appended to default compiler options. To override
   completely, see CXXFLAGS\_COMMON in src/CMakeLists.txt
 * use `cmake -DCMAKE_INSTALL_PREFIX=...` (or related variables, see
   CMakeLists.txt) to specify the target file space (default is /usr/local)
 * `make install DESTDIR=/elsewhere` copies to another target location
 * Potential tuning options for smaller binaries, build analysis, ...

```
   -DEXTRA_LIBS="-lsupc++"
   -DENABLE_LTO=ON
   -DEXTRA_MSGMERGE=--verbose -DEXTRA_MSGFMT=--verbose
```

 * There is a configuration example for cmake builds in rebuild.sh.
   run: `./rebuild.sh -r --prefix=/usr`

[ vim: set ft=markdown: ]: #
