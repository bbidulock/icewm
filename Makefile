# GNU Make is required for this makefile
include ./VERSION

# Please run 'configure' first
-include ./install.mk

BINFILES    = \
	src/icewm \
        src/icewm_hint \
        src/icewm_bg \
        src/icewm_root \
        src/icewm_about \
        src/icewm_help \
        src/icewm_dock \
        src/icewm_sound \
        src/icewm_sysdlg
        
DATAFILES   = \
	lib/winoptions # lib/preferences lib/menu lib/toolbar lib/keys

DOCFILES    = \
	README TODO CHANGED COPYING FAQ INSTALL VERSION icewm.lsm
#XPMDIRS     = \
#	icons ledclock taskbar mailbox
THEMES      = nice # metal2 motif win95 warp3 warp4 gtk2
SPEC        = icewm.spec
LSM         = icewm.lsm

#GNOMEFILES  = src/icewm-gnome

all:
	cd src && $(MAKE) DATADIR=$(DATADIR) CONFDIR=$(CONFDIR)
	cd doc && $(MAKE)

docs:
	cd doc ; $(MAKE)

depend:
	cd src ; $(MAKE) depend

clean:
	cd src ; $(MAKE) clean
	cd doc ; $(MAKE) clean
	find src -name "*.d" -exec rm -f {} \;

distclean: clean
	find . -name "*~" -exec rm -f {} \;
	rm -f \
        	config.cache config.log config.status \
        	install.mk sysdep.mk

maintainer-clean: clean
	rm -f $(SPEC) $(LSM) configure
	cd doc ; $(MAKE) maintainer-clean
	find . -name "*~" -exec rm -f {} \;

dist:	$(SPEC) $(LSM) distclean docs configure

configure: configure.in
	aclocal
	autoconf
	autoheader

$(SPEC): $(SPEC).in VERSION
	@echo spec $@
	@sed <$< >$@ \
		-e "s|@@VERSION@@|$(VERSION)|g" \
		-e "s|@@PNAME@@|$(PNAME)|g"

$(LSM): $(LSM).in VERSION
	@echo lsm $@
	@sed <$< >$@ \
	    -e "s|@@VERSION@@|$(VERSION)|g" \
	    -e "s|@@DATE@@|`date +%d%b%Y | tr 'a-z' 'A-Z'`|"

# Makefile TABS *SUCK*
install: all
	@echo ------------------------------------------
	@echo Installing icewm to $(BINDIR)
	@$(INSTALLDIR) $(BINDIR)
	@for a in $(BINFILES) ; do \
            $(INSTALLBIN) $$a $(BINDIR); \
        done
	@echo Installing defaults, icons and themes to $(DATADIR)
	@$(INSTALLDIR) $(DATADIR)/$(PNAME)
	@for a in $(DATAFILES) ; do \
            $(INSTALLDATA) $$a $(DATADIR)/$(PNAME); \
        done
	@for theme in $(THEMES) ; do \
            echo Installing theme: $$theme; \
            $(INSTALLDIR) $(DATADIR)/$(PNAME)/themes/$$theme; \
            for pixmap in lib/themes/$$theme/*.xpm ; do \
                $(INSTALLDATA) $$pixmap $(DATADIR)/$(PNAME)/themes/$$theme; \
            done; \
            for config in lib/themes/$$theme/*.pref; do \
                $(INSTALLDATA) $$config $(DATADIR)/$(PNAME)/themes/$$theme; \
            done; \
        done
	## @for a in $(CONFFILES) ; do $(INSTALLCONF) $$a $(CONFDIR) ; done

	#@$(INSTALLDIR) $(ETCDIR)
            #for l in $(XPMDIRS) ; do \
            #    if [ -d lib/themes/$$theme/$$l ] ; then \
            #        $(INSTALLDIR) $(DATADIR)/themes/$$theme/$$l; \
            #        for f in lib/themes/$$theme/$$l/* ; do \
            #            $(INSTALLDATA) $$f $(DATADIR)/themes/$$theme/$$l; \
            #        done;\
            #    fi;\
            #done;\

	#@for l in $(XPMDIRS) ; do \
        #    $(INSTALLDIR) $(DATADIR)/$$l; \
        #    for f in lib/$$l/* ; do \
        #        $(INSTALLDATA) $$f $(DATADIR)/$$l; \
        #    done; \
        #done
#install-gnome:
#	@echo ------------------------------------------
#	@echo Installing icewm-gnome to $(BINDIR)
#	@$(INSTALLDIR) $(BINDIR)
#	@for a in $(GNOMEFILES) ; do \
#            $(INSTALLBIN) $$a $(BINDIR); \
#        done

#install-doc: $(LSM)
#	mkdir /usr/doc/icewm-$(VERSION)
#	cp $(DOCFILES) /usr/doc/$(PNAME)-$(VERSION)
#	cp -r doc /usr/doc/$(PNAME)-$(VERSION)
