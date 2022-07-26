
EXTRA_DIST = \
	$(html_documentation) \
	icewm.md

doc_DATA = \
	$(html_documentation)

html_documentation = \
	icewm.html

$(html_documentation): %.html: %.md %.adoc
	$(AM_V_GEN)if test -x "`which $(MARKDOWN) 2>/dev/null`"; then \
		$(MARKDOWN) $(srcdir)/$*.md >$@; else \
		$(ASCIIDOC) $(ASCIIDOC_BACKEND) -o $@ $(srcdir)/$*.adoc; fi

maintainer-clean-local:
	-rm -f -- $(html_documentation)

