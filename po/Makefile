-include ../install.inc

PACKAGE =	icewm

SOURCES =	$(shell grep -l '\<_\>' ../src/*.cc)
MOFILES =	de.mo ja.mo ru.mo pt_BR.mo es.mo zh_TW.Big5.mo fr
POFILES =	$(MOFILES:.mo=.po)
POXFILES =	$(MOFILES:.mo=.pox)

XGETTEXT =	xgettext
MSGMERGE =	msgmerge
MSGFMT =	msgfmt

.SUFFIXES:
.SUFFIXES: .po .mo

all: $(MOFILES)

install: all
	@echo Installing localized messages in $(LOCDIR)
	@for c in $(MOFILES); do \
	  LANG=`echo $$c|sed -e 's/.mo//'` ; \
	  MSGDIR="$(LOCDIR)/$$LANG/LC_MESSAGES"; \
	  echo Installing language: $$LANG ; \
	  $(INSTALLDIR) $$MSGDIR; \
	  $(INSTALLBIN) $$c $$MSGDIR/$(PACKAGE).mo; \
	done

clean:
	rm -f  $(MOFILES) *~


merge: $(POXFILES)

# POTFILES.in lists files containing translatable strings
POTFILES.in: $(SOURCES)
	grep -l '\<_\>' ../src/*.cc > $@

# $(PACKAGE).pot is a template file for translations
$(PACKAGE).pot: POTFILES.in
	test -f $(PACKAGE).pot && \
	( ( test -f $(PACKAGE).pot.orig && \
	    cmp $(PACKAGE).pot $(PACKAGE).pot.orig ) || \
	  ( cp $(PACKAGE).pot $(PACKAGE).pot.orig && \
	    chmod a-w $(PACKAGE).pot.orig ) )

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
