
icewm_man1_files = \
	icesh.man \
	icewmbg.man \
	icewmhint.man \
	icewm.man \
	icewm-session.man \
	icewm-set-gnomewm.man \
	icewmtray.man

extra_man1_files = \
	icehelp.man \
	icesound.man \
	icewm-menu-fdo.man \
	icewm-menu-gnome2.man

dist_man1_MANS = $(icewm_man1_files)

if BUILD_HELP
dist_man1_MANS += icehelp.man
endif

if BUILD_SOUND
dist_man1_MANS += icesound.man
endif

if BUILD_MENU_FDO
dist_man1_MANS += icewm-menu-fdo.man
endif

if BUILD_MENU_GNOME2
dist_man1_MANS += icewm-menu-gnome2.man
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

dist_man5_MANS = $(icewm_man5_files)

DISTCLEANFILES = $(icewm_man1_files) $(extra_man1_files) $(icewm_man5_files)

EXTRA_DIST = \
	$(extra_man1_files) \
	icehelp.pod \
	icesh.pod \
	icesound.pod \
	icewmbg.pod \
	icewm-env.pod \
	icewm-focus_mode.pod \
	icewmhint.pod \
	icewm-keys.pod \
	icewm-menu-fdo.pod \
	icewm-menu-gnome2.pod \
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

