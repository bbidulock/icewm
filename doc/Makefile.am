
EXTRA_DIST = \
	$(html_documentation) \
	icewm.adoc

doc_DATA = \
	$(html_documentation)

html_documentation = \
	icewm.html

$(html_documentation): %.html: %.md %.adoc
	if test -x "`which $(MARKDOWN) 2>/dev/null`"; then \
		ls -l $(srcdir)/icewm.* icewm.*; \
		$(MARKDOWN) $(srcdir)/$*.md >$@; else \
		ls -l $(srcdir)/icewm.* icewm.*; \
		$(ASCIIDOC) $(ASCIIDOC_BACKEND) -o $@ $(srcdir)/$*.adoc; fi

maintainer-clean-local:
	-rm -f -- $(html_documentation)

