#!/bin/sh

cd "$(dirname "$0")"

PACKAGE=$(grep AC_INIT configure.ac|head -1|sed -r 's,AC_INIT[(][[],,;s,[]].*,,')

GTVERSION=$(gettext --version|head -1|awk '{print$NF}'|sed -r 's,(^[^\.]*\.[^\.]*\.[^\.]*)\..*$,\1,')

if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
	VERSION=$(git describe --tags|sed 's,[-_],.,g;s,\.g.*$,,')
	DATE=$(git show -s --format=%ci HEAD^{commit}|awk '{print$1}')
	sed -i.bak configure.ac -r \
		-e "s:AC_INIT\([[]$PACKAGE[]],[[][^]]*[]]:AC_INIT([$PACKAGE],[$VERSION]:
		    s:AC_REVISION\([[][^]]*[]]\):AC_REVISION([$VERSION]):
		    s:^DATE=.*$:DATE='$DATE':
		    s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):"
	subst="s:%%PACKAGE%%:$PACKAGE:g
	       s:%%VERSION%%:$VERSION:g
	       s:%%DATE%%:$DATE:g"
	sed -r -e "$subst" icewm.spec.in >icewm.spec
	sed -r -e "$subst" icewm.lsm.in >icewm.lsm
	echo -e "PACKAGE=$PACKAGE\nVERSION=$VERSION" >VERSION
else
	sed -i.bak configure.ac -r \
		-e "s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):"
fi

mkdir m4 2>/dev/null

autoreconf -iv
