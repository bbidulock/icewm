include ../VERSION
-include ../install.inc
-include ../sysdep.inc

# executable extenstion
EXEEXT=

CXXFLAGS = \
	$(I18N) $(OS2)\
	-DLIBDIR='"$(LIBDIR)"' \
	-DCFGDIR='"$(CFGDIR)"' \
	-DLOCDIR='"$(LOCDIR)"' \
	-DKDEDIR='"$(KDEDIR)"' \
	-DPACKAGE='"icewm"' \
	-DVERSION='"$(VERSION)"' \
	-DRELEASE='"$(RELEASE)"' \
	-DHOSTOS='"$(shell uname -s || echo unknown)"' \
	-DHOSTCPU='"$(shell uname -m || echo unknown)"' \
	-DEXEEXT='"$(EXEEXT)"' \
	-DICEWMEXE='"icewm"EXEEXT' \
	-DICEHELPEXE='"icehelp"EXEEXT' \
	-DICEHELPIDX='"$(DOCDIR)/icewm-$(VERSION)/icewm.html"' \
        $(SYS_CFLAGS) $(SYS_INCDIRS)
LFLAGS = $(SYS_LIBDIRS) 
LIBS = $(SYS_LIBS)

COREOBJS = \
        yapp.o ytimer.o ywindow.o ypaint.o ytooltip.o ypopup.o \
        ymenu.o ymenuitem.o yscrollbar.o ybutton.o ylistbox.o yinput.o \
        misc.o icons.o ycursor.o ylabel.o yscrollview.o ysocket.o \
        wmconfig.o ypaths.o

genpref_OBJS = \
	genpref.o

icewm_LIBS = \
	$(CORE_LIBS) $(IMAGE_LIBS) $(GNOME_LIBS)
icewm_OBJS = \
        ymsgbox.o ydialog.o yurl.o \
        wmsession.o wmwinlist.o wmtaskbar.o wmwinmenu.o \
        wmdialog.o wmabout.o wmswitch.o wmstatus.o \
        wmoption.o wmaction.o \
        wmcontainer.o wmclient.o \
        wmmgr.o wmapp.o \
        wmframe.o wmbutton.o wmminiicon.o wmtitle.o movesize.o \
        themes.o decorate.o gnome.o browse.o \
        wmprog.o \
        atasks.o aworkspaces.o amailbox.o aclock.o acpustatus.o \
	apppstatus.o aaddressbar.o objbar.o aapm.o \
        $(COREOBJS)

icewmhint_LIBS = \
	$(CORE_LIBS)
icewmhint_OBJS = \
	icewmhint.o

icewmbg_LIBS = \
	$(CORE_LIBS) $(IMAGE_LIBS)
icewmbg_OBJS = \
	icewmbg.o misc.o

icesound_LIBS = \
	$(CORE_LIBS) $(AUDIO_LIBS)
icesound_OBJS = \
	icesound.o misc.o ycmdline.o

icehelp_LIBS = \
	$(CORE_LIBS) $(IMAGE_LIBS)
icehelp_OBJS = \
	$(COREOBJS) icehelp.o

iceclock_OBJS = \
	$(COREOBJS) iceclock.o aclock.o
icebar_OBJS = \
	$(COREOBJS) \
        wmtaskbar.o \
        wmprog.o gnome.o browse.o themes.o wmaction.o \
	amailbox.o aclock.o acpustatus.o apppstatus.o aaddressbar.o objbar.o
icesame_OBJS = \
	$(COREOBJS) icesame.o
icelist_OBJS = \
	$(COREOBJS) icelist.o
iceview_OBJS = \
	$(COREOBJS) iceview.o
iceicon_OBJS = \
	$(COREOBJS) iceicon.o
icerun_OBJS = \
	$(COREOBJS) icerun.o
iceskt_OBJS = \
	$(COREOBJS) iceskt.o

COREAPPS = \
	genpref$(EXEEXT) icewm$(EXEEXT) icewmhint$(EXEEXT) icewmbg$(EXEEXT) 
PROGRAMS = \
	$(COREAPPS) \
	icehelp$(EXEEXT) icesound$(EXEEXT) \
	#icelist$(EXEEXT) iceclock$(EXEEXT) icebar$(EXEEXT) icerun$(EXEEXT)


all: base

-include .targets .depends

base: $(COREAPPS) ../lib/preferences

clean:
	rm -f $(PROGRAMS) *.o *~

# for best results patch gccmakedep to use -MM instead of -M
.depends:
	@touch $@
	gccmakedep -f $@ *.cc

.targets: Makefile
	@{ for p in $(PROGRAMS); do echo $$p: "\$$($${p}_OBJS)"; done; } > $@

../lib/preferences: genpref
	./genpref >../lib/preferences

%.o: %.cc
	@echo Compiling $@...
	@$(CXX) $(CXXFLAGS) -c $<

$(PROGRAMS):
	@echo Linking $@...
	@$(LD) -o $@ $($@_OBJS) $(LFLAGS) $($@_LFLAGS) $(LIBS) $($@_LIBS)

libice.so: $(COREOBJS)
	-@rm -f $@
	ld -shared -o $@ $(COREOBJS)

wmabout.o: ../VERSION
