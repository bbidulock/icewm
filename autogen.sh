#!/bin/sh

aclocal=${ACLOCAL:-aclocal}
autoconf=${AUTOCONF:-autoconf}
autoheader=${AUTOHEADER:-autoheader}

while test $# -gt 0; do
	case $1 in
	--with-aclocal)
		shift
		aclocal=$1
		;;
	--with-aclocal=*)
		aclocal=`echo $1 | sed 's/^--with-aclocal=//'`
		;;
	--with-autoconf)
		shift
		autoconf=$1
		;;
	--with-autoconf=*)
		autoconf=`echo $1 | sed 's/^--with-autoconf=//'`
		;;
	--with-autoheader)
		shift
		autoheader=$1
		;;
	--with-autoheader=*)
		autoheader=`echo $1 | sed 's/^--with-autoheader=//'`
		;;
	--*)
		cat <<.
Usage: autogen [OPTIONS]

Options:
  --with-aclocal=PROGRAM	version of aclocal to use.
  --with-autoconf=PROGRAM	version of autoconf to use.
  --with-autoheader=PROGRAM	version of autoheader to use.

Alternatively you can the the variables ACLOCAL, AUTOCONF, AUTOHEADER.
.
		exit

		;;
   	esac

	shift
done

rm -f config.cache

. ./VERSION

sed  <icewm.spec.in >icewm.spec \
	-e 's/%%VERSION%%/'"$VERSION"'/'

sed <icewm.lsm.in >icewm.lsm \
	-e 's/%%VERSION%%/'"$VERSION"'/' \
	-e 's/%%DATE%%/'"`date +%d%b%Y`"'/'

"$aclocal" &&
"$autoconf" &&
"$autoheader" &&
echo "You can run \`configure' now to create your Makefile." ||
cat >&2 <<.
Failed to build the \`configure' script. You need GNU autoconf version 2.50
(or newer) installed for this procedure.  If autoconf should be installed
allready call `basename $0` --help" to see how to adjust this script.
.

#
# !!! Fix the build system to allow $top_builddir != $top_srcdir
# !!! or add an option to create build directories (find, ln -s, ...)
#
# echo "Maybe you want to create a build directory first."
#
