# GNU Make is required for this makefile
include ./VERSION

PNAME       = icewm-cvs

# Please run 'configure' first
-include ./install.inc

BINFILES    = src/$(PNAME) src/$(PNAME)-hint src/$(PNAME)-bg
DATAFILES    = lib/winoptions # lib/preferences lib/menu lib/toolbar lib/keys
DOCFILES    = README TODO CHANGED COPYING FAQ INSTALL VERSION icewm.lsm
XPMDIRS     = icons ledclock taskbar mailbox
THEMES      = nice # metal2 motif win95 warp3 warp4 gtk2
SPEC        = icewm.spec
LSM         = icewm.lsm

#GNOMEFILES  = src/icewm-gnome

all:
	cd src && $(MAKE) DATADIR=$(DATADIR) PNAME=$(PNAME) ETCDIR=$(ETCDIR)
	cd doc && $(MAKE)

docs:
	cd doc ; $(MAKE)

depend:
	cd src ; $(MAKE) depend

srcclean:
	cd src ; $(MAKE) depclean

clean:  srcclean
	cd doc ; $(MAKE) clean

distclean: clean
	rm -f *~ config.cache config.log config.status install.inc \
	sysdep.inc lib/preferences


maintainer-clean: clean
	rm -f $(SPEC) $(LSM) configure
	cd doc ; $(MAKE) maintainer-clean

dist:	$(SPEC) $(LSM) distclean docs depend configure

configure: configure.in
	autoconf

$(SPEC): $(SPEC).in VERSION
	sed -e "s|@@VERSION@@|$(VERSION)|g" <$< >$@

$(LSM): $(LSM).in VERSION
	sed <$< >$@ \
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
	@for a in $(DATAFILES) ; do \
            $(INSTALLDATA) $$a $(DATADIR); \
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
	#@for a in $(ETCFILES) ; do $(INSTALLETC) $$a $(ETCDIR) ; done

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

install-doc: $(LSM)
	mkdir /usr/doc/icewm-$(VERSION)
	cp $(DOCFILES) /usr/doc/icewm-$(VERSION)
	cp -r doc /usr/doc/icewm-$(VERSION)
