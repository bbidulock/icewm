
html_documentation = icewm.html

EXTRA_DIST = \
	$(html_documentation) \
	icewm.md \
	icewm.adoc

doc_DATA = $(html_documentation)

mark_input = $(srcdir)/icewm.md
adoc_input = $(srcdir)/icewm.adoc

$(html_documentation):
	$(AM_V_GEN)if test -x "`which $(MARKDOWN) 2>/dev/null`"; then \
		if test $(mark_input) -nt $(html_documentation); then \
			$(MARKDOWN) $(mark_input) >$@; \
		fi; \
	elif test -x "`which $(ASCIIDOC) 2>/dev/null`"; then \
		if test $(adoc_input) -nt $(html_documentation); then \
			$(ASCIIDOC) $(ASCIIDOC_BACKEND) -o $@ $(adoc_input); \
		fi; \
	else \
		echo "Please install markdown or asciidoctor." >&2; \
		exit 1; \
	fi

maintainer-clean-local:
	-rm -f -- $(html_documentation)

