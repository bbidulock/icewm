
EXTRA_DIST = \
	$(html_documentation) \
	icewm.adoc

doc_DATA = \
	$(html_documentation)

html_documentation = \
	icewm.html

$(html_documentation): %.html: %.adoc
	$(AM_V_GEN)$(ASCIIDOC) $(ASCIIDOC_BACKEND) -o $@ $<

maintainer-clean-local:
	-rm -f -- $(html_documentation)

