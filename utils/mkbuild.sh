#!/bin/sh

error() {
    rc=$1; shift
    echo $* >&2
    exit $rc
}

BUILDDIR=${1:-../build}
SOURCEDIR=${2:-$PWD}

[ ! -d "$SOURCEDIR" ] && error 1\
	"Invalid source directory: \`$SOURCEDIR'"
[ -e "$BUILDDIR" ] && error 1\
	"Build directory exists: \`$BUILDDIR'"
[ "$SOURCEDIR" -ef "$BUILDDIR" ] && error 1\
	"Identical source and build directory: \`$SOURCEDIR' and \`$BUILDDIR'"

find $SOURCEDIR -type d -not -name CVS \
	-printf $BUILDDIR/%P\\0 | xargs -0 mkdir -p
find $SOURCEDIR -type f -not -path \*/CVS/\* \
	-printf "ln -s %p $BUILDDIR/%P\\n" | sh
