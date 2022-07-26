
EXTRA_DIST = \
	$(html_documentation) \
	icewm.adoc

doc_DATA = \
	$(html_documentation)

html_documentation = \
	icewm.html

$(html_documentation): %.html: %.md %.adoc
	$(AM_V_GEN)if test -x "`which $(MARKDOWN) 2>/dev/null`"; then \
		ls -l icewm.*; \
		$(MARKDOWN) $*.md >$@; else \
		ls -l icewm.*; \
		$(ASCIIDOC) $(ASCIIDOC_BACKEND) -o $@ $*.adoc; fi

maintainer-clean-local:
	-rm -f -- $(html_documentation)

