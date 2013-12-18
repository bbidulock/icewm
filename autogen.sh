#!/bin/sh

rm -f config.cache

[ -f ./VERSION ] && . ./VERSION

if [ -d `dirname "$0"`/.git -a -x "`which git 2>/dev/null`" ]
then
	VERSION=`git describe --tags --always | sed 's|^[^0-9]*||;s|[-_]|.|g;s|[.]g[a-f0-9]*$||'`
fi

cp -f configure.ac configure.in

sed <configure.in >configure.ac \
	-e "s|^AC_INIT(.*$|AC_INIT([icewm], [$VERSION], [http://github.com/bbidulock/icewm])|"

rm -f configure.in

sed <icewm.spec.in >icewm.spec \
	-e 's/%%VERSION%%/'"$VERSION"'/'

sed <icewm.lsm.in >icewm.lsm \
	-e 's/%%VERSION%%/'"$VERSION"'/' \
	-e 's/%%DATE%%/'"`date +%d%b%Y`"'/'

autoreconf -fiv

