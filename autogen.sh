#!/bin/sh

rm -f config.cache

aclocal
autoconf
autoheader

echo "You can run \`configure' now to create your Makefile."

#
# !!! Fix the build system to allow $top_builddir != $top_srcdir
# !!! or add an option to create build directories (find, ln -s, ...)
#
# echo "Maybe you want to create a build directory first."
#
