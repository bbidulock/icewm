include ../VERSION
-include ../install.inc
-include ../sysdep.inc

#DEBUG       = -DDEBUG

# executable extenstion
EXEEXT=

CXXFLAGS    = \
	$(I18N) $(DEBUG) $(OS2)\
	-DLIBDIR='"$(LIBDIR)"' \
	-DCONFIGDIR='"$(CONFIGDIR)"' \
	-DKDEDATADIR='"$(KDEDATADIR)"' \
	-DPACKAGE='"icewm"' \
	-DVERSION='"$(VERSION)"' \
	-DEXEEXT='"$(EXEEXT)"' \
	-DICEWMEXE='"icewm"EXEEXT' \
        $(SYS_CFLAGS) $(SYS_INCDIRS)
LFLAGS      = 
LIBS        = $(SYS_LIBDIRS) $(SYS_LIBS)

COREOBJS    = \
        yapp.o ytimer.o ywindow.o ypaint.o ytooltip.o ypopup.o \
        ymenu.o ymenuitem.o yscrollbar.o ybutton.o ylistbox.o yinput.o \
        misc.o icons.o ylabel.o yscrollview.o ysocket.o \
        wmconfig.o

ICEWMOBJS        = \
        ymsgbox.o ydialog.o \
        wmsession.o wmwinlist.o wmtaskbar.o wmwinmenu.o \
        wmdialog.o wmabout.o wmswitch.o wmstatus.o \
        wmoption.o wmaction.o \
        wmcontainer.o wmclient.o \
        wmmgr.o wmapp.o \
        wmframe.o wmbutton.o wmminiicon.o wmtitle.o movesize.o \
        themes.o decorate.o gnome.o browse.o \
        wmprog.o \
        atasks.o aworkspaces.o amailbox.o aclock.o acpustatus.o apppstatus.o aaddressbar.o objbar.o aapm.o \
        $(COREOBJS)

GENPREFOBJS = genpref.o

ICEWMHINTOBJS = icewmhint.o

ICEWMBGOBJS = icewmbg.o

ICESOUNDOBJS = icesound.o misc.o 

ICECLOCKOBJS = $(COREOBJS) \
          iceclock.o aclock.o

ICEBAROBJS = $(COREOBJS) \
          wmtaskbar.o \
          wmprog.o gnome.o browse.o themes.o wmaction.o \
	  amailbox.o aclock.o acpustatus.o apppstatus.o aaddressbar.o objbar.o

ICESAMEOBJS = $(COREOBJS) \
          icesame.o

ICELISTOBJS = $(COREOBJS) \
             icelist.o

ICEVIEWOBJS = $(COREOBJS) \
             iceview.o

ICEICONOBJS = $(COREOBJS) \
             iceicon.o

ICEHELPOBJS = $(COREOBJS) \
             icehelp.o

ICERUNOBJS = $(COREOBJS) \
             icerun.o

ICESKTOBJS = $(COREOBJS) \
             iceskt.o

PROGRAMS    = icewm$(EXEEXT) genpref$(EXEEXT) icewmhint$(EXEEXT) icewmbg$(EXEEXT) #icelist$(EXEEXT) #iceclock$(EXEEXT) #icebar$(EXEEXT) #icesound$(EXEEXT) #icerun$(EXEEXT)

.SUFFIXES:
.SUFFIXES: .cc .o .h

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<

all: $(PROGRAMS) ../lib/preferences

clean:
	rm -f $(PROGRAMS) *.o *~

../lib/preferences: genpref
	./genpref >../lib/preferences

icewm$(EXEEXT): $(ICEWMOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMOBJS) $(LIBS)

genpref$(EXEEXT): $(GENPREFOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(GENPREFOBJS)

icewmhint$(EXEEXT): $(ICEWMHINTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMHINTOBJS) $(LIBS)

icewmbg$(EXEEXT): $(ICEWMBGOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEWMBGOBJS) $(LIBS)

icesound$(EXEEXT): $(ICESOUNDOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESOUNDOBJS) $(LIBS)

iceclock$(EXEEXT): $(ICECLOCKOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICECLOCKOBJS) $(LIBS)

icebar$(EXEEXT): $(ICEBAROBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEBAROBJS) $(LIBS)

icesame$(EXEEXT): $(ICESAMEOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESAMEOBJS) $(LIBS)

icelist$(EXEEXT): $(ICELISTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICELISTOBJS) $(LIBS)

iceview$(EXEEXT): $(ICEVIEWOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEVIEWOBJS) $(LIBS)

iceicon$(EXEEXT): $(ICEICONOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEICONOBJS) $(LIBS)

icehelp$(EXEEXT): $(ICEHELPOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICEHELPOBJS) $(LIBS)

icerun$(EXEEXT): $(ICERUNOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICERUNOBJS) $(LIBS)

iceskt$(EXEEXT): $(ICESKTOBJS)
	-@rm -f $@
	$(LD) -o $@ $(LFLAGS) $(ICESKTOBJS) $(LIBS)

libice.so: $(COREOBJS)
	-@rm -f $@
	ld -shared -o $@ $(COREOBJS)

# for best results patch gccmakedep to use -MM instead of -M
depend:
	gccmakedep *.cc

# DO NOT DELETE
aaddressbar.o: aaddressbar.cc config.h ykey.h ylib.h base.h debug.h \
 aaddressbar.h
aapm.o: aapm.cc config.h ylib.h base.h debug.h sysdep.h aapm.h \
 ywindow.h ypaint.h ytimer.h aclock.h yapp.h prefs.h default.h \
 bindkey.h intl.h
aclock.o: aclock.cc config.h ylib.h base.h debug.h aclock.h ywindow.h \
 ypaint.h ytimer.h yapp.h prefs.h default.h bindkey.h
acpustatus.o: acpustatus.cc config.h
amailbox.o: amailbox.cc config.h intl.h
apppstatus.o: apppstatus.cc config.h ylib.h base.h debug.h yapp.h \
 ywindow.h ypaint.h apppstatus.h
atasks.o: atasks.cc config.h
aworkspaces.o: aworkspaces.cc config.h
browse.o: browse.cc config.h obj.h ypaint.h objmenu.h ymenu.h ypopup.h \
 ywindow.h ytimer.h base.h debug.h ymenuitem.h yaction.h browse.h \
 wmmgr.h WinMgr.h wmprog.h sysdep.h
decorate.o: decorate.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmframe.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmaction.h wmtitle.h wmapp.h yapp.h wmcontainer.h ymenuitem.h
genpref.o: genpref.cc config.h ykey.h ylib.h base.h debug.h sysdep.h \
 bindkey.h default.h intl.h
gnome.o: gnome.cc config.h
iceclock.o: iceclock.cc config.h yapp.h ywindow.h ypaint.h aclock.h \
 ytimer.h base.h debug.h
icehelp.o: icehelp.cc config.h ylib.h base.h debug.h ylistbox.h \
 ywindow.h ypaint.h yscrollbar.h ytimer.h yscrollview.h ymenu.h \
 ypopup.h yapp.h sysdep.h yaction.h ymenuitem.h prefs.h default.h \
 bindkey.h intl.h
iceicon.o: iceicon.cc config.h ylib.h base.h debug.h ywindow.h \
 ypaint.h yscrollbar.h ytimer.h yscrollview.h ymenu.h ypopup.h yapp.h \
 yaction.h wmmgr.h WinMgr.h sysdep.h
icelist.o: icelist.cc config.h ylib.h base.h debug.h ylistbox.h \
 ywindow.h ypaint.h yscrollbar.h ytimer.h yscrollview.h ymenu.h \
 ypopup.h yapp.h yaction.h yinputline.h wmmgr.h WinMgr.h sysdep.h \
 intl.h
icerun.o: icerun.cc config.h yapp.h ywindow.h ypaint.h yinputline.h \
 ytimer.h base.h debug.h yaction.h ylabel.h ybutton.h prefs.h \
 default.h bindkey.h
icesame.o: icesame.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 ywindow.h ypaint.h ylabel.h ymenuitem.h ymenu.h ypopup.h ytimer.h \
 yapp.h yaction.h MwmUtil.h intl.h
icesound.o: icesound.cc config.h guievent.h
iceview.o: iceview.cc config.h ylib.h base.h debug.h ylistbox.h \
 ywindow.h ypaint.h yscrollbar.h ytimer.h yscrollview.h ymenu.h \
 ypopup.h yapp.h sysdep.h yaction.h intl.h
icewmbg.o: icewmbg.cc config.h intl.h base.h debug.h WinMgr.h
icewmhint.o: icewmhint.cc config.h guievent.h intl.h
icons.o: icons.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 ypaint.h yapp.h ywindow.h sysdep.h prefs.h default.h bindkey.h \
 wmprog.h objmenu.h ymenu.h ypopup.h ytimer.h ymenuitem.h yaction.h \
 obj.h intl.h
misc.o: misc.cc config.h sysdep.h ylib.h base.h debug.h intl.h
movesize.o: movesize.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmframe.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmstatus.h yapp.h
objbar.o: objbar.cc config.h
testwinhints.o: testwinhints.cc WinMgr.h
themes.o: themes.cc config.h themes.h objmenu.h ymenu.h ypopup.h \
 ywindow.h ypaint.h ytimer.h base.h debug.h ymenuitem.h yaction.h \
 obj.h wmmgr.h WinMgr.h wmprog.h sysdep.h prefs.h default.h bindkey.h \
 intl.h
wmabout.o: wmabout.cc config.h ylib.h base.h debug.h wmabout.h \
 ydialog.h wmclient.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h \
 MwmUtil.h ylabel.h prefs.h default.h bindkey.h wmapp.h yapp.h \
 ymsgbox.h yactionbutton.h ybutton.h yaction.h wmframe.h wmbutton.h \
 wmoption.h WinMgr.h wmmgr.h sysdep.h intl.h
wmaction.o: wmaction.cc config.h wmaction.h yaction.h
wmapp.o: wmapp.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 atasks.h ywindow.h ypaint.h wmclient.h ymenu.h ypopup.h ytimer.h \
 MwmUtil.h wmapp.h yapp.h ymsgbox.h ydialog.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmaction.h wmmgr.h WinMgr.h wmframe.h wmbutton.h \
 wmoption.h prefs.h default.h bindkey.h wmprog.h objmenu.h ymenuitem.h \
 obj.h wmswitch.h wmstatus.h wmabout.h wmdialog.h wmconfig.h \
 wmwinmenu.h wmwinlist.h ylistbox.h yscrollbar.h yscrollview.h \
 wmtaskbar.h apppstatus.h wmsession.h gnomeapps.h browse.h themes.h \
 sysdep.h intl.h
wmbutton.o: wmbutton.cc config.h ylib.h base.h debug.h wmbutton.h \
 yactionbutton.h ybutton.h ywindow.h ypaint.h yaction.h ymenu.h \
 ypopup.h ytimer.h wmaction.h wmframe.h ymsgbox.h ydialog.h wmclient.h \
 MwmUtil.h ylabel.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmtitle.h yapp.h
wmclient.o: wmclient.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmclient.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h MwmUtil.h \
 wmframe.h ymsgbox.h ydialog.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmapp.h yapp.h sysdep.h
wmconfig.o: wmconfig.cc config.h ykey.h ylib.h base.h debug.h \
 wmoption.h ypaint.h wmmgr.h ywindow.h ymenu.h ypopup.h ytimer.h \
 WinMgr.h sysdep.h prefs.h default.h bindkey.h yaction.h intl.h
wmcontainer.o: wmcontainer.cc config.h ylib.h base.h debug.h \
 wmcontainer.h ywindow.h ypaint.h wmframe.h ymenu.h ypopup.h ytimer.h \
 ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h \
 WinMgr.h wmmgr.h yapp.h
wmdialog.o: wmdialog.cc config.h ykey.h ylib.h base.h debug.h \
 wmdialog.h ywindow.h ypaint.h yaction.h wmaction.h yactionbutton.h \
 ybutton.h prefs.h default.h bindkey.h wmapp.h yapp.h ymenu.h ypopup.h \
 ytimer.h ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h wmmgr.h \
 WinMgr.h intl.h
wmframe.o: wmframe.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmframe.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h atasks.h wmaction.h wmcontainer.h wmtitle.h wmminiicon.h \
 wmswitch.h wmtaskbar.h apppstatus.h wmwinlist.h ylistbox.h \
 yscrollbar.h yscrollview.h wmapp.h yapp.h sysdep.h intl.h
wmmgr.o: wmmgr.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmmgr.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h WinMgr.h \
 aaddressbar.h atasks.h wmclient.h MwmUtil.h aworkspaces.h ybutton.h \
 sysdep.h wmtaskbar.h yaction.h apppstatus.h wmwinlist.h ylistbox.h \
 yscrollbar.h yscrollview.h objmenu.h ymenuitem.h obj.h wmswitch.h \
 wmstatus.h wmminiicon.h wmcontainer.h wmframe.h ymsgbox.h ydialog.h \
 ylabel.h yactionbutton.h wmbutton.h wmoption.h prefs.h default.h \
 bindkey.h wmdialog.h wmsession.h wmapp.h yapp.h wmaction.h wmprog.h
wmminiicon.o: wmminiicon.cc config.h ylib.h base.h debug.h \
 wmminiicon.h ywindow.h ypaint.h wmframe.h ymenu.h ypopup.h ytimer.h \
 ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h \
 WinMgr.h wmmgr.h yapp.h
wmoption.o: wmoption.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmoption.h wmframe.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h \
 ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h \
 ybutton.h yaction.h wmbutton.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h sysdep.h intl.h
wmprog.o: wmprog.cc config.h ykey.h ylib.h base.h debug.h objmenu.h \
 ymenu.h ypopup.h ywindow.h ypaint.h ytimer.h ymenuitem.h yaction.h \
 obj.h wmprog.h wmoption.h wmaction.h ybutton.h objbutton.h objbar.h \
 prefs.h default.h bindkey.h wmapp.h yapp.h ymsgbox.h ydialog.h \
 wmclient.h MwmUtil.h ylabel.h yactionbutton.h sysdep.h wmmgr.h \
 WinMgr.h gnomeapps.h themes.h browse.h wmtaskbar.h apppstatus.h \
 intl.h
wmsession.o: wmsession.cc config.h
wmstatus.o: wmstatus.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 wmstatus.h ywindow.h ypaint.h wmswitch.h ymenu.h ypopup.h ytimer.h \
 wmframe.h ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h \
 yactionbutton.h ybutton.h yaction.h wmbutton.h wmoption.h prefs.h \
 default.h bindkey.h WinMgr.h wmmgr.h intl.h
wmswitch.o: wmswitch.cc config.h ykey.h ylib.h base.h debug.h \
 wmswitch.h ymenu.h ypopup.h ywindow.h ypaint.h ytimer.h wmmgr.h \
 WinMgr.h wmframe.h ymsgbox.h ydialog.h wmclient.h MwmUtil.h ylabel.h \
 yactionbutton.h ybutton.h yaction.h wmbutton.h wmoption.h prefs.h \
 default.h bindkey.h yapp.h
wmtaskbar.o: wmtaskbar.cc config.h
wmtitle.o: wmtitle.cc config.h ylib.h base.h debug.h wmtitle.h \
 ywindow.h ypaint.h wmframe.h ymenu.h ypopup.h ytimer.h ymsgbox.h \
 ydialog.h wmclient.h MwmUtil.h ylabel.h yactionbutton.h ybutton.h \
 yaction.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmwinlist.h ylistbox.h yscrollbar.h yscrollview.h wmapp.h \
 yapp.h
wmwinlist.o: wmwinlist.cc config.h ykey.h ylib.h base.h debug.h \
 wmwinlist.h wmclient.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h \
 MwmUtil.h ylistbox.h yscrollbar.h yscrollview.h yaction.h ymenuitem.h \
 wmaction.h wmframe.h ymsgbox.h ydialog.h ylabel.h yactionbutton.h \
 ybutton.h wmbutton.h wmoption.h prefs.h default.h bindkey.h WinMgr.h \
 wmmgr.h wmapp.h yapp.h sysdep.h intl.h
wmwinmenu.o: wmwinmenu.cc config.h
yapp.o: yapp.cc config.h yfull.h ykey.h ylib.h base.h debug.h yapp.h \
 ywindow.h ypaint.h ysocket.h sysdep.h wmprog.h objmenu.h ymenu.h \
 ypopup.h ytimer.h ymenuitem.h yaction.h obj.h wmmgr.h WinMgr.h \
 MwmUtil.h prefs.h default.h bindkey.h intl.h
ybutton.o: ybutton.cc config.h ykey.h ylib.h base.h debug.h ybutton.h \
 ywindow.h ypaint.h yaction.h ymenu.h ypopup.h ytimer.h yapp.h prefs.h \
 default.h bindkey.h wmtaskbar.h wmclient.h MwmUtil.h apppstatus.h \
 intl.h
ydialog.o: ydialog.cc config.h ykey.h ylib.h base.h debug.h ydialog.h \
 wmclient.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h MwmUtil.h \
 yapp.h prefs.h default.h bindkey.h WinMgr.h wmframe.h ymsgbox.h \
 ylabel.h yactionbutton.h ybutton.h yaction.h wmbutton.h wmoption.h \
 wmmgr.h wmdialog.h wmabout.h sysdep.h
yinput.o: yinput.cc config.h ykey.h ylib.h base.h debug.h yinputline.h \
 ywindow.h ypaint.h ytimer.h yaction.h ymenu.h ypopup.h ymenuitem.h \
 yapp.h prefs.h default.h bindkey.h intl.h
ylabel.o: ylabel.cc config.h ylabel.h ywindow.h ypaint.h ymenu.h \
 ypopup.h ytimer.h base.h debug.h prefs.h default.h bindkey.h
ylistbox.o: ylistbox.cc config.h ykey.h ylib.h base.h debug.h \
 ylistbox.h ywindow.h ypaint.h yscrollbar.h ytimer.h yscrollview.h \
 ymenu.h ypopup.h yapp.h prefs.h default.h bindkey.h
ymenu.o: ymenu.cc config.h ykey.h ylib.h base.h debug.h ymenu.h \
 ypopup.h ywindow.h ypaint.h ytimer.h yaction.h ymenuitem.h yapp.h \
 prefs.h default.h bindkey.h
ymenuitem.o: ymenuitem.cc config.h ykey.h ylib.h base.h debug.h \
 ymenuitem.h ymenu.h ypopup.h ywindow.h ypaint.h ytimer.h yaction.h \
 yapp.h prefs.h default.h bindkey.h
ymsgbox.o: ymsgbox.cc config.h ylib.h base.h debug.h ymsgbox.h \
 ydialog.h wmclient.h ywindow.h ypaint.h ymenu.h ypopup.h ytimer.h \
 MwmUtil.h ylabel.h yactionbutton.h ybutton.h yaction.h WinMgr.h \
 yapp.h wmframe.h wmbutton.h wmoption.h prefs.h default.h bindkey.h \
 wmmgr.h sysdep.h intl.h
ypaint.o: ypaint.cc config.h ylib.h base.h debug.h ypaint.h yapp.h \
 ywindow.h sysdep.h prefs.h default.h bindkey.h intl.h
ypopup.o: ypopup.cc config.h ykey.h ylib.h base.h debug.h ymenu.h \
 ypopup.h ywindow.h ypaint.h ytimer.h yapp.h prefs.h default.h \
 bindkey.h
yscrollbar.o: yscrollbar.cc config.h ykey.h ylib.h base.h debug.h \
 yscrollbar.h ywindow.h ypaint.h ytimer.h yapp.h prefs.h default.h \
 bindkey.h
yscrollview.o: yscrollview.cc config.h yscrollview.h ywindow.h \
 ypaint.h ylistbox.h yscrollbar.h ytimer.h base.h debug.h yapp.h \
 prefs.h default.h bindkey.h
ysocket.o: ysocket.cc config.h base.h debug.h ysocket.h yapp.h \
 ywindow.h ypaint.h
ytimer.o: ytimer.cc config.h ytimer.h base.h debug.h yapp.h ywindow.h \
 ypaint.h
ytooltip.o: ytooltip.cc config.h ytooltip.h
ywindow.o: ywindow.cc config.h yfull.h ykey.h ylib.h base.h debug.h \
 ywindow.h ypaint.h ytooltip.h yapp.h sysdep.h prefs.h default.h \
 bindkey.h ytimer.h
