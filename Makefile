################################################################################
# GNU Make is required for this makefile
################################################################################
# Please run 'configure' first (generate it with autogen.sh)
################################################################################

include ./VERSION
-include ./install.inc

################################################################################

BINFILES    = $(foreach APP,$(APPLICATIONS),src/$(APP))
LIBFILES    = lib/preferences lib/menu lib/toolbar lib/winoptions lib/keys
DOCFILES    = README TODO CHANGES COPYING AUTHORS FAQ INSTALL VERSION icewm.lsm
XPMDIRS     = icons ledclock taskbar mailbox cursors
THEMES      = nice motif win95 warp3 warp4 metal2 gtk2 Infadel2
SPEC        = icewm.spec
LSM         = icewm.lsm

GNOMEFILES  = lib/IceWM.desktop

WMPROPDIR   = $(shell gnome-config --datadir)/gnome/wm-properties/

all:		base docs nls
install:	 install-base install-docs install-nls

depend base icesound icehelp:
	@$(MAKE) -C src $@ LIBDIR=$(LIBDIR) CFGDIR=$(CFGDIR)

docs:
	@$(MAKE) -C doc all

nls:
	@$(MAKE) -C po all

gnome:

srcclean:
	@$(MAKE) -C src clean

clean:  srcclean
	@$(MAKE) -C doc clean

distclean: clean
	rm -f *~ config.cache config.log config.status install.inc \
	sysdep.inc src/config.h lib/preferences


maintainer-clean: distclean
	rm -f $(SPEC) $(LSM) Makefile configure src/config.h.in
	@$(MAKE) -C doc maintainer-clean

dist:	$(SPEC) $(LSM) distclean docs depend configure

configure: configure.in
	aclocal
	autoconf
	autoheader

$(SPEC): $(SPEC).in VERSION
	sed  <$< >$@ \
	    -e "s|@@VERSION@@|$(VERSION)|g" \
	    -e "s|@@RELEASE@@|$(RELEASE)|g"

$(LSM): $(LSM).in VERSION
	sed <$< >$@ \
	    -e "s|@@VERSION@@|$(VERSION)|g" \
	    -e "s|@@DATE@@|`date +%d%b%Y | tr 'a-z' 'A-Z'`|"

# Makefile TABS *SUCK*
install-base: base
	@echo ------------------------------------------
	@echo "Installing binaries in $(BINDIR)"
	@$(INSTALLDIR) $(BINDIR)
	@for app in $(BINFILES) ; do \
            $(INSTALLBIN) "$$app" $(BINDIR); \
        done
	@echo "Installing defaults, icons and themes in $(LIBDIR)"
	@$(INSTALLDIR) $(LIBDIR)
	@$(INSTALLDIR) $(CFGDIR)
	@for lib in $(LIBFILES) ; do \
            $(INSTALLLIB) "$$lib" $(LIBDIR); \
        done
	@for xpmdir in $(XPMDIRS); do \
	    if test -d lib/$$xpmdir; then \
		$(INSTALLDIR) $(LIBDIR)/$$xpmdir; \
		for pixmap in lib/$$xpmdir/*.xpm ; do \
		    $(INSTALLLIB) "$$pixmap" $(LIBDIR)/$$xpmdir; \
		done; \
	    fi; \
        done
	@echo ------------------------------------------
	@for theme in $(THEMES) ; do \
            echo "Installing theme: $$theme"; \
            $(INSTALLDIR) $(LIBDIR)/themes/$$theme; \
            for pixmap in lib/themes/$$theme/*.xpm; do \
                $(INSTALLLIB) "$$pixmap" $(LIBDIR)/themes/$$theme; \
            done; \
            for subtheme in lib/themes/$$theme/*.theme; do \
                $(INSTALLLIB) "$$subtheme" $(LIBDIR)/themes/$$theme; \
            done; \
	    if test -f lib/themes/$$theme/*.pcf; then \
                for fontfile in lib/themes/$$theme/*.pcf; do \
		    $(INSTALLLIB) "$$fontfile" $(LIBDIR)/themes/$$theme; \
		done; \
		if test -x "$(MKFONTDIR)"; then \
		  cwd=$$PWD; cd "$(LIBDIR)/themes/$$theme"; \
		  $(MKFONTDIR); \
		  cd $$cwd; \
		else \
		  echo "WARNING: A dummy file has been copied to"; \
		  echo "         $(LIBDIR)/themes/$$theme/fonts.dir"; \
		  echo "         You better setup your path to point to mkfontdir or use the"; \
		  echo "         --with-mkfontdir option of the configure script."; \
		  $(INSTALLLIB) "lib/themes/$$theme/fonts.dir.default" \
				"$(LIBDIR)/themes/$$theme/fonts.dir"; \
		fi; \
	    fi; \
            for xpmdir in $(XPMDIRS) ; do \
                if [ -d lib/themes/$$theme/$$xpmdir ] ; then \
                    $(INSTALLDIR) $(LIBDIR)/themes/$$theme/$$xpmdir; \
                    for f in lib/themes/$$theme/$$xpmdir/*.xpm ; do \
                        $(INSTALLLIB) "$$f" $(LIBDIR)/themes/$$theme/$$xpmdir; \
                    done;\
                fi;\
            done;\
        done
	@#for a in $(ETCFILES) ; do $(INSTALLETC) "$$a" $(CFGDIR) ; done
	@echo ------------------------------------------

install-docs: docs $(LSM)
	@echo ------------------------------------------
	@rm -fr $(DOCDIR)/icewm-$(VERSION)
	@$(INSTALLDIR) $(DOCDIR)/icewm-$(VERSION)
	@echo Installing documentation in $(DOCDIR); \
	cp $(DOCFILES) $(DOCDIR)/icewm-$(VERSION); \
	cp -r doc/*.sgml doc/*.html $(DOCDIR)/icewm-$(VERSION)
	@echo ------------------------------------------

install-nls: nls
	@echo ------------------------------------------
	@$(MAKE) -C po install
	@echo ------------------------------------------

install-gnome: gnome
	@echo ------------------------------------------
	@echo Installing icewm-gnome in $(WMPROPDIR)
	@$(INSTALLDIR) $(WMPROPDIR)
	@$(INSTALLLIB) lib/IceWM.desktop $(WMPROPDIR)
	@echo ------------------------------------------
