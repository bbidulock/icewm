# GNU Make is required for this makefile
include ./VERSION
# Please run 'configure' first
-include ./install.inc

BINFILES    = src/icewm #src/icewmhint src/icewmbg
LIBFILES    = lib/preferences lib/menu lib/toolbar lib/winoptions lib/keys
DOCFILES    = README TODO CHANGED COPYING FAQ INSTALL VERSION icewm.lsm
XPMDIRS     = icons ledclock taskbar mailbox
THEMES      = nice motif win95 warp3 warp4 metal2 gtk2
SPEC        = icewm.spec
LSM         = icewm.lsm

GNOMEFILES  = src/icewm-gnome

all:
	cd src ; $(MAKE) LIBDIR=$(LIBDIR) ETCDIR=$(ETCDIR)
	cd doc ; $(MAKE)

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
	@echo Installing defaults, icons and themes to $(LIBDIR)
	@$(INSTALLDIR) $(LIBDIR)
	@$(INSTALLDIR) $(ETCDIR)
	@for a in $(LIBFILES) ; do \
            $(INSTALLLIB) $$a $(LIBDIR); \
        done
	@for l in $(XPMDIRS) ; do \
            $(INSTALLDIR) $(LIBDIR)/$$l; \
            for f in lib/$$l/* ; do \
                $(INSTALLLIB) $$f $(LIBDIR)/$$l; \
            done; \
        done
	@for theme in $(THEMES) ; do \
            echo Installing theme: $$theme; \
            $(INSTALLDIR) $(LIBDIR)/themes/$$theme; \
            for pixmap in lib/themes/$$theme/*.xpm ; do \
                $(INSTALLLIB) $$pixmap $(LIBDIR)/themes/$$theme; \
            done; \
            for config in lib/themes/$$theme/*.theme ; do \
                $(INSTALLLIB) $$config $(LIBDIR)/themes/$$theme; \
            done; \
            for l in $(XPMDIRS) ; do \
                if [ -d lib/themes/$$theme/$$l ] ; then \
                    $(INSTALLDIR) $(LIBDIR)/themes/$$theme/$$l; \
                    for f in lib/themes/$$theme/$$l/* ; do \
                        $(INSTALLLIB) $$f $(LIBDIR)/themes/$$theme/$$l; \
                    done;\
                fi;\
            done;\
        done
	@#for a in $(ETCFILES) ; do $(INSTALLETC) $$a $(ETCDIR) ; done

install-gnome:
	@echo ------------------------------------------
	@echo Installing icewm-gnome to $(BINDIR)
	@$(INSTALLDIR) $(BINDIR)
	@for a in $(GNOMEFILES) ; do \
            $(INSTALLBIN) $$a $(BINDIR); \
        done

install-doc: $(LSM)
	mkdir /usr/doc/icewm-$(VERSION)
	cp $(DOCFILES) /usr/doc/icewm-$(VERSION)
	cp -r doc /usr/doc/icewm-$(VERSION)
