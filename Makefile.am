# Makefile.am for IceWM - an X11 Window Manager

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src po doc lib

EXTRA_DIST = \
	INSTALL \
	COPYING \
	AUTHORS \
	NEWS \
	README \
	THANKS \
	TODO \
	ChangeLog \
	COMPLIANCE \
	VERSION \
	icewm.spec.in \
	icewm.spec \
	icewm.lsm.in \
	icewm.lsm

CLEANFILES = THANKS README README.md ChangeLog icewm.lsm icewm.spec

dist_noinst_SCRIPTS = \
	autogen.sh

dist_bin_SCRIPTS = \
	icewm-set-gnomewm

ChangeLog:
	( \
	  echo -e "# created with git log -n 200 --abbrev-commit --decorate --stat=76 -M -C|fmt -sct -w80\n" ; \
	  git log -n 200 --abbrev-commit --decorate --stat=76 -M -C|fmt -sct -w80 ; \
	)>$@

THANKS:
	( \
	  echo "$(PACKAGE) -- authors file.  $(DATE)" ; \
	  echo "" ; \
	  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}'|tac \
	)>$@

README.md: README.md.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

README: README.md
	cat $< | fmt -sct -w80 >$@

icewm.spec: icewm.spec.in
	sed -r -e 's:%%PACKAGE%%:$(PACKAGE):g;s:%%VERSION%%:$(VERSION):g;s:%%DATE%%:$(DATE):g' $< >$@

icewm.lsm: icewm.lsm.in
	sed -r -e 's:%%PACKAGE%%:$(PACKAGE):g;s:%%VERSION%%:$(VERSION):g;s:%%DATE%%:$(DATE):g' $< >$@
