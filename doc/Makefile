all: icewm.html

maintainer-clean: distclean
	rm -f icewm*.html

distclean: clean
	rm -f *~

clean:

icewm.html: icewm.sgml
	-sgml2html icewm.sgml

