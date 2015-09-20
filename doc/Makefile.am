
html_documentation = \
	icewm.html \
	icewm-1.html \
	icewm-2.html \
	icewm-3.html \
	icewm-4.html \
	icewm-5.html \
	icewm-6.html \
	icewm-7.html \
	icewm-8.html \
	icewm-9.html \
	icewm-10.html \
	icewm-11.html \
	icewm-12.html \
	icewm-13.html \
	icewm-14.html \
	icewm-15.html \
	icewm-16.html \
	icewm-17.html \
	icewm-18.html \
	icewm-19.html \
	icewm-20.html

dist_doc_DATA = \
	icewm.sgml \
	techinfo.sgml \
	$(html_documentation)

dist_man1_MANS = \
	icewm.man


$(html_documentation): icewm.sgml
	-sgml2html $<

CLEANFILES = $(html_documentation)

