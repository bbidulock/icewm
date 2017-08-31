
icewm_man_files = \
	icewm.man \
	icewmbg.man \
	icewm-session.man \
	icewmtray.man

dist_man1_MANS = $(icewm_man_files)
DISTCLEANFILES = $(icewm_man_files)

EXTRA_DIST = \
	icewm.pod \
	icewmbg.pod \
	icewm-session.pod \
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

