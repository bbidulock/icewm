#!/bin/sh

PACKAGE=icewm
VERSION=
DATE="`date -uI`"

[ -f ./VERSION ] && . ./VERSION

if [ -x "`which git 2>/dev/null`" -a -d .git ]; then
	DATE=$(git log --date=iso|grep '^Date:'|head -1|awk '{print$2}')
	VERSION=$(git describe --tags|sed 's,^[^0-9]*,,;s,[-_],.,g;s,\.g.*$,,')
	(
	   echo -e "# created with git log -n 200 --abbrev-commit --decorate --stat=76 -M -C|fmt -sct -w80\n"
	   git log -n 200 --abbrev-commit --decorate --stat=76 -M -C|fmt -sct -w80
	)>ChangeLog
	(
	   echo "$PACKAGE -- authors file.  $DATE"
	   echo ""
	   git log|grep '^Author:'|awk '{if(!authors[$0]){print$0;authors[$0]=1;}}'|tac
	)>THANKS
fi

GTVERSION=`gettext --version|head -1|awk '{print$NF}'|sed -r 's,(^[^\.]*\.[^\.]*\.[^\.]*)\..*$,\1,'`

cp -f configure.ac configure.in

sed <configure.in >configure.ac -r \
	-e "s:AC_INIT\([[]$PACKAGE[]],[[][^]]*[]]:AC_INIT([$PACKAGE],[$VERSION]:
	    s:AC_REVISION\([[][^]]*[]]\):AC_REVISION([$VERSION]):
	    s:^DATE=.*$:DATE='$DATE':
	    s:^AM_GNU_GETTEXT_VERSION.*:AM_GNU_GETTEXT_VERSION([$GTVERSION]):"

rm -f configure.in

subst="s:@PACKAGE@:$PACKAGE:g
       s:@VERSION@:$VERSION:g
       s:@DATE@:$DATE:g"

sed -r -e "$subst" README.md.in >README.md
fmt -sct -w 80 README.md >README

subst="s:%%PACKAGE%%:$PACKAGE:g
       s:%%VERSION%%:$VERSION:g
       s:%%DATE%%:$DATE:g"

sed -r -e "$subst" icewm.spec.in >icewm.spec
sed -r -e "$subst" icewm.lsm.in >icewm.lsm

autoreconf -iv

