-include ../install.inc

PACKAGE =	icewm

SOURCES =	$(shell grep -l '\<_\>' ../src/*.cc)
POFILES =	$(wildcard *.po)
POXFILES =	$(POFILES:.po=.pox)
MOFILES =	$(POFILES:.po=.mo)
#de.mo ja.mo ru.mo pt_BR.mo es.mo zh_TW.Big5.mo fr.mo

XGETTEXT =	xgettext
MSGMERGE =	msgmerge --indent --verbose
MSGFMT =	msgfmt --check --statistics --verbose

.SUFFIXES:
.SUFFIXES: .po .mo

all: $(MOFILES)

install: all
	@echo Installing message catalogues in $(LOCDIR)
	@for c in $(MOFILES); do \
	  LANG=`echo $$c|sed -e 's/.mo//'` ; \
	  MSGDIR="$(LOCDIR)/$$LANG/LC_MESSAGES"; \
	  echo Installing language: $$LANG ; \
	  $(INSTALLDIR) $$MSGDIR; \
	  $(INSTALLLIB) $$c $$MSGDIR/$(PACKAGE).mo; \
	done

clean:
	rm -f  $(MOFILES) *~


merge: $(POXFILES)

# POTFILES.in lists files containing translatable strings
POTFILES.in: $(SOURCES)
	grep -l '\<_\>' ../src/*.cc > $@

# $(PACKAGE).pot is a template file for translations
$(PACKAGE).pot: POTFILES.in
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=../src \
	--add-comments --keyword=_ --keyword=N_ --files-from=POTFILES.in && \
	test ! -f $(PACKAGE).po || \
	( rm -f ./$(PACKAGE).pot && \
	  mv $(PACKAGE).po ./$(PACKAGE).pot )

# create new translations
%.pox: %.po $(PACKAGE).pot
	$(MSGMERGE) --indent --output-file $@ $< $(PACKAGE).pot

# convert portable into machine objects
.po.mo:
	$(MSGFMT) -o $@ $<


report.html: *.po Makefile
	@(echo "<h1>National Language Support Status Report</h1>"       ;\
	  date; echo "<p>"                                              ;\
	  for c in *.po							;\
	  do	echo -n "<li>$$c"					;\
	 	sed -ne's|^.*"Last-Translator[^:]*:\(.*\)<.*$$| by\1<ul>|p' $$c	;\
		echo -n ' <li>' 					;\
		LC_ALL=en $(MSGFMT) -o /dev/null $$c 2>&1		|\
		sed -e's|, |<li>|g' -e's|\.$$||'			;\
		echo '</ul>'						;\
	  done ) >$@
	@cat $@

upload-report: report.html
	scp report.html massel@icewm.sf.net:icesf/libphp/nls.html
