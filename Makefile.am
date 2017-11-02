
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src po doc man lib

EXTRA_DIST = INSTALL COPYING AUTHORS NEWS README README.md README.md.in THANKS TODO ChangeLog COMPLIANCE

dist_noinst_SCRIPTS = gennews.sh autogen.sh configure.sh rebuild.sh dist.sh

DISTCLEANFILES = ChangeLog AUTHORS NEWS README README.md README.html README.txt

dist-hook:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		chmod u+w $(distdir)/{ChangeLog,AUTHORS,NEWS} ; \
		( cd $(srcdir) ; \
		  echo -e "# created with git log --stat=76 -M -C -500|fmt -sct -w80\n" ; \
		  git log --stat=76 -M -C -500|fmt -sct -w80 \
		)|head -10000>$(distdir)/ChangeLog ; \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- authors file.  $(DATE)" ; \
		  echo "" ; \
		  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}' \
		)>$(distdir)/AUTHORS ; \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- history of user-visible changes.  $(DATE)" ; \
		  PACKAGE="$(PACKAGE)" $(srcdir)/gennews.sh \
		)>$(distdir)/NEWS ; \
	fi ; \
	chmod u+w $(distdir)/{README.md,README} ; \
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g;s:\@BRANCH\@:$(BRANCH):g' $(distdir)/README.md.in >$(distdir)/README.md ; \
	cat $(distdir)/README.md | fmt -sct -w80 -g72 >$(distdir)/README

ChangeLog:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo -e "# created with git log --stat=76 -M -C -500|fmt -sct -w80\n" ; \
		  git log --stat=76 -M -C -500|fmt -sct -w80 \
		)|head -10000>$@ ; \
	else touch $@ ; fi

AUTHORS:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- authors file.  $(DATE)" ; \
		  echo "" ; \
		  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}' \
		)>$@ ; \
	else touch $@ ; fi

NEWS:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- history of user-visible changes.  $(DATE)" ; \
		  PACKAGE="$(PACKAGE)" $(srcdir)/gennews.sh \
		)>$@ ; \
	else touch $@ ; fi

README.md: README.md.in
	$(AM_V_GEN)sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g;s:\@BRANCH\@:$(BRANCH):g' $< >$@

README: README.md
	$(AM_V_GEN)cat $< | fmt -sct -w80 -g72 >$@

README.html: README.md
	$(AM_V_GEN)$(MARKDOWN) $< >$@

README.txt: README.html
	$(AM_V_GEN)$(LINKS) -dump -width 80 $< >$@

INSTALL.cmakebuild: INSTALL-cmakebuild.md
	$(AM_V_GEN)cat $< | fmt -sct -w80 -g72 >$@

INSTALL-cmakebuild.html: INSTALL-cmakebuild.md
	$(AM_V_GEN)$(MARKDOWN) $< >$@

INSTALL-cmakebuild.txt: INSTALL-cmakebuild.html
	$(AM_V_GEN)links -dump -width 80 -codepage utf-8 $< >$@

EXTRA_DIST += \
	INSTALL-cmakebuild.md \
	INSTALL.cmakebuild \
	CMakeLists.txt \
	doc/CMakeLists.txt \
	lib/CMakeLists.txt \
	po/CMakeLists.txt \
	src/CMakeLists.txt \
	src/config.cmake.h.in

EXTRA_DIST += COMPLIANCE

EXTRA_DIST += VERSION

DISTCLEANFILES += VERSION

VERSION:
	$(AM_V_GEN)echo -e "PACKAGE=$(PACKAGE)\nVERSION=$(VERSION)" >$@

EXTRA_DIST += icewm.spec.in icewm.spec icewm.lsm.in icewm.lsm

DISTCLEANFILES += icewm.spec icewm.lsm

icewm.spec: icewm.spec.in
	$(AM_V_GEN)sed -r -e 's:%%PACKAGE%%:$(PACKAGE):g;s:%%VERSION%%:$(VERSION):g;s:%%DATE%%:$(DATE):g;s:%%BRANCH%%:$(BRANCH):g' $< >$@

icewm.lsm: icewm.lsm.in
	$(AM_V_GEN)sed -r -e 's:%%PACKAGE%%:$(PACKAGE):g;s:%%VERSION%%:$(VERSION):g;s:%%DATE%%:$(DATE):g;s:%%BRANCH%%:$(BRANCH):g' $< >$@

dist_bin_SCRIPTS = \
	icewm-set-gnomewm \
	contrib/icewm-menu-xrandr

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = \
	contrib/icewm-menu-xrandr

