all: icewm.html

maintainer-clean: distclean
	rm -f *.html
	rm -fr api

distclean: clean
	rm -f *~

clean:

icewm.html: icewm.sgml
	-sgml2html icewm.sgml

apidoc:
	-doxygen
	-cd api; doxytag -s search.idx
