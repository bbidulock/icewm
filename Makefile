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
        
DATADIRS    = \
	icewm

DATAFILES   = \
	winoptions \
        icewm/close.xpm \
        icewm/hide.xpm \
        icewm/maximize.xpm \
        icewm/menu.xpm \
        icewm/minimize.xpm \
        icewm/restore.xpm \
        icewm/rolldown.xpm \
        icewm/rollup.xpm

THEMES      = \
	nice # metal2 motif win95 warp3 warp4 gtk2
        
DOCFILES    = \
	README TODO CHANGED COPYING FAQ INSTALL VERSION icewm.lsm
        
#XPMDIRS     = \
#	icons ledclock taskbar mailbox
SPEC        = icewm.spec
LSM         = icewm.lsm

all:
	cd src && $(MAKE) DATADIR=$(DATADIR) CONFDIR=$(CONFDIR)
	cd doc && $(MAKE)

docs:
	cd doc && $(MAKE)

depend:
	cd src && $(MAKE) depend

clean:
	cd src && $(MAKE) clean
	cd doc && $(MAKE) clean
	find src -name "*.d" -exec rm -f {} \;

distclean: clean
	rm -f \
        	config.cache config.log config.status \
        	install.mk sysdep.mk

maintainer-clean: distclean
	rm -f $(SPEC) $(LSM) configure
	cd doc ; $(MAKE) maintainer-clean
	find . -name "*~" -exec rm -f {} \;

dist:	maintainer-clean $(SPEC) $(LSM) docs configure

configure: configure.in
	aclocal
	autoconf
	autoheader

$(SPEC): $(SPEC).in VERSION
	@echo spec $@
	@sed <$< >$@ \
		-e "s|@@VERSION@@|$(VERSION)|g"

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
	@$(INSTALLDIR) $(DATADIR)
	@for d in $(DATADIRS) ; do \
	    $(INSTALLDIR) $(DATADIR)/$$d; \
        done
	@for a in $(DATAFILES) ; do \
            $(INSTALLDATA) lib/$$a $(DATADIR)/$$a; \
        done
	@for theme in $(THEMES) ; do \
            echo Installing theme: $$theme; \
            $(INSTALLDIR) $(DATADIR)/themes/$$theme; \
            for pixmap in lib/themes/$$theme/*.xpm ; do \
                $(INSTALLDATA) $$pixmap $(DATADIR)/themes/$$theme; \
            done; \
            for config in lib/themes/$$theme/*.pref; do \
                $(INSTALLDATA) $$config $(DATADIR)/themes/$$theme; \
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

#install-doc: $(LSM)
#	mkdir /usr/doc/icewm-$(VERSION)
#	cp $(DOCFILES) /usr/doc/$(PNAME)-$(VERSION)
#	cp -r doc /usr/doc/$(PNAME)-$(VERSION)
