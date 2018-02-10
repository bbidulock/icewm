
icewm_man1_files = \
	icehelp.man \
	icesh.man \
	icewmbg.man \
	icewmhint.man \
	icewm.man \
	icewm-session.man \
	icewm-set-gnomewm.man \
	icewmtray.man

icewm_man1_htmls = $(icewm_man1_files:%.man=%.1.html)

doc_DATA = $(icewm_man1_htmls)

extra_man1_files = \
	icesound.man \
	icewm-menu-fdo.man \
	icewm-menu-xrandr.man

extra_man1_htmls = $(extra_man1_files:%.man=%.1.html)

man1_MANS = $(icewm_man1_files)

if BUILD_SOUND
man1_MANS += icesound.man
doc_DATA += icesound.1.html
endif

if BUILD_MENU_FDO
man1_MANS += icewm-menu-fdo.man
doc_DATA += icewm-menu-fdo.1.html
endif

icewm_man5_files = \
	icewm-env.man \
	icewm-focus_mode.man \
	icewm-keys.man \
	icewm-menu.man \
	icewm-preferences.man \
	icewm-prefoverride.man \
	icewm-programs.man \
	icewm-shutdown.man \
	icewm-startup.man \
	icewm-theme.man \
	icewm-toolbar.man \
	icewm-winoptions.man

icewm_man5_htmls = $(icewm_man5_files:%.man=%.5.html)

man5_MANS = $(icewm_man5_files)
doc_DATA += $(icewm_man5_htmls)

CLEANFILES = \
	$(icewm_man1_files) $(extra_man1_files) $(icewm_man5_files) \
	$(icewm_man1_htmls) $(extra_man1_htmls) $(icewm_man5_htmls) *.tmp

EXTRA_DIST = \
	icehelp.pod \
	icesh.pod \
	icesound.pod \
	icewmbg.pod \
	icewm-env.pod \
	icewm-focus_mode.pod \
	icewmhint.pod \
	icewm-keys.pod \
	icewm-menu-fdo.pod \
	icewm-menu-xrandr.pod \
	icewm-menu.pod \
	icewm.pod \
	icewm-preferences.pod \
	icewm-prefoverride.pod \
	icewm-programs.pod \
	icewm-session.pod \
	icewm-set-gnomewm.pod \
	icewm-shutdown.pod \
	icewm-startup.pod \
	icewm-theme.pod \
	icewm-toolbar.pod \
	icewmtray.pod \
	icewm-winoptions.pod

$(icewm_man1_htmls) $(extra_man1_htmls): %.1.html: %.pod
	$(AM_V_GEN)pod2html \
		--backlink \
		--header \
		--poderrors \
		--index \
		--podroot=$(top_builddir) \
		--podpath=man \
		--htmldir=$(docdir) \
		--noindex \
		--infile=$< \
		--quiet \
		--title="User Commands" | \
		$(SED) -r -e 's,http://man.he.net/man(.)/([^"]*),\2.\1.html,g' >$@

$(icewm_man5_htmls): %.5.html: %.pod
	$(AM_V_GEN)pod2html \
		--backlink \
		--header \
		--poderrors \
		--index \
		--podroot=$(top_builddir) \
		--podpath=man \
		--htmldir=$(docdir) \
		--noindex \
		--infile=$< \
		--quiet \
		--title="Standards, Environments and Macros" | \
		$(SED) -r -e 's,http://man.he.net/man(.)/([^"]*),\2.\1.html,g' >$@

$(icewm_man1_files) $(extra_man1_files): %.man: %.pod
	$(AM_V_GEN)pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@

$(icewm_man5_files): %.man: %.pod
	$(AM_V_GEN)pod2man --center="Standards, Environments and Macros" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="5" \
		$< >$@

