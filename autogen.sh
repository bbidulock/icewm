#!/bin/sh

if [ "$(uname)" = SunOS ]; then
    alias gettext=ggettext
fi

cd "$(dirname "$0")"

PACKAGE=$(grep AC_INIT configure.ac|head -1|sed -r 's,AC_INIT[(][[],,;s,[]].*,,')

GTVERSION=$(gettext --version|head -1|awk '{print$NF}'|sed -r 's,(^[^\.]*\.[^\.]*)\.[^\.]*$,\1,;s,(^[^\.]*\.[^\.]*\.[^\.]*)\.[^\.]*$,\1,')

if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
	VERSION_VERSION=$(grep -s ^VERSION= VERSION | sed 's|VERSION=||')
	VERSION_RAW=$(git describe --tags || echo ${VERSION_VERSION:-1.2.3.4})
	VERSION=$(echo $VERSION_RAW | sed 's,[-_],.,g;s,\.g.*$,,')
	DATE=$(git show -s --format=%ci HEAD^{commit}|awk '{print$1}')
	MDOCDATE=$(date --date="$DATE" +'%B %-d, %Y' 2>/dev/null || date +'%B %-d, %Y')
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
		    s:^MDOCDATE=.*$:MDOCDATE='$MDOCDATE':
		    s:^BRANCH=.*$:BRANCH='$BRANCH':
		    s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):
		    s:^AM_INIT_AUTOMAKE\([[](gnits )?:AM_INIT_AUTOMAKE([$GNITS:" \
		configure.ac
	subst="s:%%PACKAGE%%:$PACKAGE:g
	       s:%%VERSION%%:$VERSION:g
	       s:%%DATE%%:$DATE:g
	       s:%%MDOCDATE%%:$MDOCDATE:g
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

# cscope target won't work without this
#
[ -f po/Makefile.in.in ] && printf '\n%%:\n\t@:\n\n' >> po/Makefile.in.in
