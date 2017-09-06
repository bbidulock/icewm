
SUFFIXES = .adoc .html

.adoc.html:
	$(AM_V_GEN)$(ASCIIDOC) $(ASCIIDOC_BACKEND) $<

html_documentation = \
	icewm.html \
	icewm.1.html \
	icewmbg.1.html \
	icesound.1.html

EXTRA_DIST = \
	icewm.adoc \
	icewm.1.adoc \
	icewmbg.1.adoc \
	icesound.1.adoc

dist_doc_DATA = \
	$(html_documentation)

DISTCLEANFILES = $(html_documentation)

