
SUFFIXES = .1 .1.adoc .adoc .html

.1.adoc.1:
	$(ASCIIMAN) $(ASCIIMAN_FORMAT) $<

.adoc.html:
	$(ASCIIDOC) $(ASCIIDOC_BACKEND) $<

html_documentation = \
	icewm.html \
	icewm.1.html \
	icewmbg.1.html \
	icesound.1.html

generated_manpages = \
	icewm.1 \
	icewmbg.1 \
	icesound.1

dist_man1_MANS = \
	$(generated_manpages)

EXTRA_DIST = \
	icewm.adoc \
	icewm.1.adoc \
	icewmbg.1.adoc \
	icesound.1.adoc

dist_doc_DATA = \
	$(html_documentation)

DISTCLEANFILES = $(html_documentation) $(generated_manpages)

