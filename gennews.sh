#!/bin/sh

# A little script to automagically generate a GNU NEWS file from git.

if [ -z "$PACKAGE" -a -f configure.ac ]; then
	PACKAGE=$(grep AC_INIT configure.ac|head -1|sed -r 's,AC_INIT[(][[],,;s,[]].*,,')
fi

t=
i=0

for o in $(git tag --sort=-creatordate) ""; do
	if [ $i -ge 8 ]; then break; fi
	i=$((i+1))
	if [ -z "$t" ] ; then
		head=$(git show -s --format=%H HEAD)
		last=$(git show -s --format=%H "$o^{commit}")
		if [ "$head" = "$last" ]; then
			t="$o"
			continue
		fi
		t="HEAD"
		version=$(git describe --tags)
	else
		version="$t"
	fi
	version=$(echo "$version"|sed 's,^[^0-9]*,,;s,[-_],.,g;s,\.g.*$,,')
	date=$(git show -s --format=%ci "$t^{commit}"|awk '{print$1}')
	title="Release ${PACKAGE}${PACKAGE:+-}$version released $date"
	under=$(echo "$title"|sed 's,.,-,g')
	/usr/bin/echo -e "\n$title\n$under\n"
	cmd="git notes --ref=release show $t"
	notes=$(eval $cmd 2>/dev/null|sed 's,^,    ,')
	lines=$(echo "$notes"|wc -l)
	if [ $lines -gt 1 ] ; then
		/usr/bin/echo -e "$cmd\n\n$notes\n"
	fi
	if [ :$t != :HEAD ] ; then
		cmd="git tag -n100 -l ${t}"
		notes=$(eval $cmd 2>/dev/null)
		lines=$(echo "$notes"|wc -l)
		if [ $lines -gt 1 ] ; then
			/usr/bin/echo -e "$cmd\n\n$notes\n"
		fi
	fi
	cmd="git shortlog -e -n -w80,6,8 ${o}${o:+...}${t}"
	/usr/bin/echo -e "$cmd\n\n$(eval $cmd)\n"
	t="$o"
done|sed -r 's,[[:space:]][[:space:]]*$,,'

