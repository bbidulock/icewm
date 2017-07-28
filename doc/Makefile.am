
html_documentation = \
	icewm.html

dist_doc_DATA = \
	icewm.adoc \
	$(html_documentation)

dist_man1_MANS = \
	icewm.man


$(html_documentation): icewm.adoc
	$(ASCIIDOC) --backend=html4 $<

CLEANFILES = $(html_documentation)

