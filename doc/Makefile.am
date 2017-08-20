
SUFFIXES = .1 .1.adoc .adoc .html

#
# Convert from '.1.adoc' to '.1' manpage.
# The option "-D ." is needed for distcheck.
# a2x generates a spurious warning on it.
#
.1.adoc.1:
	$(ASCIIMAN) -D . $(ASCIIMAN_FORMAT) $<

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

man1_MANS = \
	$(generated_manpages)

dist_doc_DATA = \
	icewm.adoc \
	icewm.1.adoc \
	icewmbg.1.adoc \
	icesound.1.adoc \
	$(html_documentation)

CLEANFILES = $(html_documentation) $(generated_manpages)

