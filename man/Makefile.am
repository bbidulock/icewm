
icewm_man_files = \
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

dist_man1_MANS = $(icewm_man_files)
DISTCLEANFILES = $(icewm_man_files)

EXTRA_DIST = \
	icehelp.pod \
	icesh.pod \
	icesound.pod \
	icewmbg.pod \
	icewmhint.pod \
	icewm-menu-fdo.pod \
	icewm.pod \
	icewm-session.pod \
	icewm-set-gnomewm.pod \
	icewmtray.pod

SUFFIXES = .man .pod

.pod.man:
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

