
icewm_man1_files = \
	icehelp.man \
	icesh.man \
	icesound.man \
	icewmbg.man \
	icewmhint.man \
	icewm.man \
	icewm-menu-fdo.man \
	icewm-session.man \
	icewm-set-gnomewm.man \
	icewmtray.man

dist_man1_MANS = $(icewm_man1_files)

icewm_man5_files = \
	icewm-keys.man \
	icewm-menu.man \
	icewm-preferences.man \
	icewm-programs.man \
	icewm-toolbar.man \
	icewm-winoptions.man

dist_man5_MANS = $(icewm_man5_files)

DISTCLEANFILES = $(icewm_man1_files) $(icewm_man5_files)

EXTRA_DIST = \
	icehelp.pod \
	icesh.pod \
	icesound.pod \
	icewmbg.pod \
	icewmhint.pod \
	icewm-keys.pod \
	icewm-menu-fdo.pod \
	icewm-menu.pod \
	icewm.pod \
	icewm-preferences.pod \
	icewm-programs.pod \
	icewm-session.pod \
	icewm-set-gnomewm.pod \
	icewm-toolbar.pod \
	icewmtray.pod \
	icewm-winoptions.pod

$(icewm_man1_files): %.man: %.pod
	pod2man --center="User Commands" \
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
	pod2man --center="User Commands" \
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

