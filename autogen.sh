#!/bin/sh

cd "$(dirname "$0")"

PACKAGE=$(grep AC_INIT configure.ac|head -1|sed -r 's,AC_INIT[(][[],,;s,[]].*,,')

GTVERSION=$(gettext --version|head -1|awk '{print$NF}'|sed -r 's,(^[^\.]*\.[^\.]*\.[^\.]*)\..*$,\1,')

if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
	VERSION=$(git describe --tags|sed 's,[-_],.,g;s,\.g.*$,,')
	DATE=$(git show -s --format=%ci HEAD^{commit}|awk '{print$1}')
	BRANCH=$(git tag --sort=-creatordate|head -1)
	GNITS="gnits "
	if [ "$VERSION" != "$BRANCH" ]; then
		BRANCH="icewm-1-4-BRANCH"
		GNITS=""
	fi
	sed -i.bak -r \
		-e "s:AC_INIT\([[]$PACKAGE[]],[[][^]]*[]]:AC_INIT([$PACKAGE],[$VERSION]:
		    s:AC_REVISION\([[][^]]*[]]\):AC_REVISION([$VERSION]):
		    s:^DATE=.*$:DATE='$DATE':
		    s:^BRANCH=.*$:BRANCH='$BRANCH':
		    s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):
		    s:^AM_INIT_AUTOMAKE\([[](gnits )?:AM_INIT_AUTOMAKE([$GNITS:" \
		configure.ac
	subst="s:%%PACKAGE%%:$PACKAGE:g
	       s:%%VERSION%%:$VERSION:g
	       s:%%DATE%%:$DATE:g
	       s:%%BRANCH%%:$BRANCH:g"
	sed -r -e "$subst" icewm.spec.in >icewm.spec
	sed -r -e "$subst" icewm.lsm.in >icewm.lsm
	printf "PACKAGE=%s\nVERSION=%s\n" "$PACKAGE" "$VERSION" >VERSION
else
	sed -i.bak configure.ac -r \
		-e "s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):"
fi

mkdir m4 2>/dev/null

autoreconf -fiv
