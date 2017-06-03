
AM_CPPFLAGS = -include ../config.h
AM_CXXFLAGS = $(CORE_CFLAGS) $(XSM_CFLAGS) $(IMAGE_CFLAGS) $(AUDIO_CFLAGS) $(GNOME1_CFLAGS) $(GNOME2_CFLAGS) $(GIO_CFLAGS) -DEXEEXT=$(EXEEXT)

EXTRA_DIST = \
	ylibrary.cc \
	ylibrary.h \
	ybase.h \
	ypointer.h \
	globit.c

EXTRA_PROGRAMS = \
	icehelp \
	icesound \
	icewm-menu-gnome1 \
	icewm-menu-gnome2 \
	icewm-menu-fdo \
	testarray \
	testlocale \
	testmap \
	testmenus \
	testnetwmhints \
	testwinhints \
	iceview \
	icesame \
	iceicon \
	icerun \
	icelist \
	icebar \
	strtest \
	iceskt \
	iceclock

noinst_LTLIBRARIES = \
	libice.la\
	libitk.la

bin_PROGRAMS = \
	icewm \
	icewm-session \
	icesh \
	icewmhint \
	icewmbg \
	icewmtray

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = \
	icewm-session \
	icesh \
	icewmhint \
	icewmbg \
	icewmtray


noinst_PROGRAMS = \
	genpref

if BUILD_TESTS
noinst_PROGRAMS += \
	testarray \
	testlocale \
	testmap \
	testmenus \
	testnetwmhints \
	testwinhints \
	iceview \
	icesame \
	iceicon \
	icerun \
	icelist \
	icebar \
	strtest
endif

if BUILD_HELP
bin_PROGRAMS += icehelp
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += icehelp
endif

if BUILD_SOUND
bin_PROGRAMS += icesound
endif

if BUILD_MENU_GNOME2
bin_PROGRAMS += icewm-menu-gnome2
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += icewm-menu-gnome2
endif

if BUILD_MENU_FDO
bin_PROGRAMS += icewm-menu-fdo
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += icewm-menu-fdo
endif

libice_la_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	ypoll.h \
	yutil.h \
	ylib.h \
	ykey.h \
	yfull.h \
	ref.cc \
	ref.h \
	ystring.h \
	mstring.cc \
	mstring.h \
	upath.cc \
	upath.h \
	udir.cc \
	udir.h \
	yapp.cc \
	yapp.h \
	MwmUtil.h \
	yxapp.cc \
	yxapp.h \
	ytimer.cc \
	ytimer.h \
	ascii.h \
	ywindow.cc \
	ywindow.h \
	ypaint.cc \
	ypaint.h \
	ypopup.cc \
	ypopup.h \
	misc.cc \
	ycursor.cc \
	ycursor.h \
	ysocket.cc \
	ysocket.h \
	ypaths.cc \
	ypaths.h \
	ypixbuf.cc \
	ypixbuf.h \
	ylocale.cc \
	ylocale.h \
	yarray.cc \
	yarray.h \
	ypipereader.cc \
	ypipereader.h \
	yxembed.cc \
	yxembed.h \
	binascii.h \
	yconfig.cc \
	yconfig.h \
	yprefs.cc \
	yprefs.h \
	yfont.cc \
	yfontcore.cc \
	yfontxft.cc \
	ypixmap.cc \
	ypixmap.h \
	yimage.cc \
	yimage.h \
	yimage_gdk.cc \
	yimage_imlib.cc \
	yimage_xpm.cc \
	ytooltip.cc \
	ytooltip.h

libitk_la_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	ascii.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	yaction.h \
	yinputline.h \
	ylib.h \
	ypoint.h \
	yrect.h \
	ykey.h \
	yfull.h \
	ymenu.cc \
	ymenu.h \
	ylabel.cc \
	ylabel.h \
	yscrollview.cc \
	yscrollview.h \
	ymenuitem.cc \
	ymenuitem.h \
	yscrollbar.cc \
	yscrollbar.h \
	ascii.h \
	ybutton.cc \
	ybutton.h \
	ascii.h \
	ylistbox.cc \
	ylistbox.h \
	yinput.cc \
	globit.cc \
	globit.h \
	yicon.cc \
	yicon.h \
	wmconfig.cc \
	wmconfig.h
libitk_la_LIBADD = libice.la

genpref_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	genpref.cc
genpref_LDADD = misc.$(OBJEXT)

icewm_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	obj.h \
	yaction.h \
	yinputline.h \
	yactionbutton.h \
	objmenu.h \
	objbutton.h \
	WinMgr.h \
	ypoll.h \
	yutil.h \
	ylib.h \
	ymsgbox.cc \
	ymsgbox.h \
	ydialog.cc \
	ydialog.h \
	binascii.h \
	yurl.cc \
	yurl.h \
	ypoint.h \
	yrect.h \
	wmsession.cc \
	wmsession.h \
	wmwinlist.cc \
	wmwinlist.h \
	wmtaskbar.cc \
	wmtaskbar.h \
	wmwinmenu.cc \
	wmwinmenu.h \
	wmdialog.cc \
	wmdialog.h \
	wmabout.cc \
	wmabout.h \
	wmswitch.cc \
	wmswitch.h \
	wmstatus.cc \
	wmstatus.h \
	wmoption.cc \
	wmoption.h \
	wmaction.cc \
	wmaction.h \
	wmcontainer.cc \
	wmcontainer.h \
	wmclient.cc \
	MwmUtil.h \
	wmclient.h \
	wmmgr.cc \
	wmmgr.h \
	appnames.h \
	wmapp.cc \
	guievent.h \
	wmapp.h \
	wmframe.cc \
	wmframe.h \
	wmbutton.cc \
	wmbutton.h \
	wmminiicon.cc \
	wmminiicon.h \
	wmtitle.cc \
	wmtitle.h \
	movesize.cc \
	ascii.h \
	themes.cc \
	themes.h \
	decorate.cc \
	browse.cc \
	browse.h \
	wmprog.cc \
	wmprog.h \
	atasks.cc \
	atasks.h \
	aworkspaces.cc \
	aworkspaces.h \
	amailbox.cc \
	amailbox.h \
	aclock.cc \
	aclock.h \
	acpustatus.cc \
	acpustatus.h \
	amemstatus.cc \
	amemstatus.h \
	apppstatus.cc \
	apppstatus.h \
	aaddressbar.cc \
	aaddressbar.h \
	objbar.cc \
	objbar.h \
	aapm.cc \
	aapm.h \
	atray.cc \
	atray.h \
	ysmapp.cc \
	ysmapp.h \
	yxtray.cc \
	yxtray.h
icewm_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(XSM_LIBS) $(CORE_LIBS)

icesh_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	WinMgr.h \
	ylib.h \
	icesh.cc
icesh_LDADD = libice.la $(CORE_LIBS)

icewm_session_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	icesm.cc
icewm_session_LDADD = libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icewmhint_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	guievent.h \
	icewmhint.cc
icewmhint_LDADD = misc.$(OBJEXT) $(CORE_LIBS)

icewmbg_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	themable.h \
	default.h \
	WinMgr.h \
	ykey.h \
	yfull.h \
	icewmbg.cc \
	icewmbg_prefs.h
icewmbg_LDADD = libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icesound_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	guievent.h \
	ylib.h \
	icesound.cc \
	ascii.h \
	ycmdline.cc \
	ycmdline.h
icesound_LDADD = libice.la $(AUDIO_LIBS) $(CORE_LIBS)

icewm_menu_gnome1_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	themable.h \
	default.h \
	ylib.h \
	gnome.cc \
	ascii.h \
	ycmdline.cc \
	ycmdline.h
icewm_menu_gnome1_LDADD = libice.la $(GNOME1_LIBS) $(CORE_LIBS)

icewm_menu_gnome2_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	themable.h \
	default.h \
	ylib.h \
	gnome2.cc \
	ascii.h \
	ycmdline.cc \
	ycmdline.h
icewm_menu_gnome2_LDADD = libice.la $(GNOME2_LIBS) $(CORE_LIBS)

icewm_menu_fdo_SOURCES = \
	sysdep.h \
	base.h \
	fdomenu.cc
icewm_menu_fdo_LDADD = misc.$(OBJEXT) $(GIO_LIBS)

icehelp_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	ascii.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	yaction.h \
	ylib.h \
	ypoint.h \
	yrect.h \
	icehelp.cc
icehelp_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

iceclock_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	iceclock.cc \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	aclock.cc \
	aclock.h
iceclock_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icebar_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	obj.h \
	yaction.h \
	objmenu.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	objbutton.h \
	ylib.h \
	wmtaskbar.cc \
	wmtaskbar.h \
	wmprog.cc \
	wmprog.h \
	browse.cc \
	browse.h \
	ascii.h \
	themes.cc \
	themes.h \
	wmaction.cc \
	wmaction.h \
	amailbox.cc \
	amailbox.h \
	aclock.cc \
	aclock.h \
	acpustatus.cc \
	acpustatus.h \
	amemstatus.cc \
	amemstatus.h \
	apppstatus.cc \
	apppstatus.h \
	aaddressbar.cc \
	aaddressbar.h \
	objbar.cc \
	objbar.h
#	icebar.cc
icebar_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

strtest_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	udir.h \
	strtest.cc \
	mstring.h
strtest_LDADD = libice.la

icewmtray_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	yxtray.cc \
	yxtray.h \
	ylib.h \
	icetray.cc
icewmtray_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icesame_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	MwmUtil.h \
	yaction.h \
	ypoint.h \
	yrect.h \
	ykey.h \
	yfull.h \
	icesame.cc
icesame_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icelist_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	yaction.h \
	yinputline.h \
	ypoint.h \
	yrect.h \
	ykey.h \
	yfull.h \
	icelist.cc
icelist_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

iceview_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	yaction.h \
	ylib.h \
	ypoint.h \
	yrect.h \
	iceview.cc
iceview_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

iceicon_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	yaction.h \
	ylib.h \
	ypoint.h \
	yrect.h \
	iceicon.cc
iceicon_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

icerun_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	yaction.h \
	yinputline.h \
	icerun.cc
icerun_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

iceskt_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	iceskt.cc
iceskt_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

testmenus_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	obj.h \
	yaction.h \
	testmenus.cc \
	bindkey.h \
	themable.h \
	default.h \
	prefs.h \
	wmprog.cc \
	wmprog.h \
	wmaction.cc \
	wmaction.h \
	ascii.h \
	themes.cc \
	themes.h \
	browse.cc \
	browse.h
testmenus_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS)

testwinhints_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	WinMgr.h \
	testwinhints.cc
testwinhints_LDFLAGS = $(IMAGE_LIBS) $(CORE_LIBS)

testnetwmhints_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	WinMgr.h \
	testnetwmhints.cc
testnetwmhints_LDFLAGS = $(IMAGE_LIBS) $(CORE_LIBS)

testmap_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	WinMgr.h \
	testmap.cc
testmap_LDFLAGS = $(IMAGE_LIBS) $(CORE_LIBS)

testlocale_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	ylib.h \
	testlocale.cc
testlocale_LDADD = libice.la

testarray_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	ylib.h \
	testarray.cc
testarray_LDADD = libice.la

nodist_pkgdata_DATA = \
	preferences

preferences: genpref$(EXEEXT)
	./genpref$(EXEEXT) >$@

CLEANFILES = preferences

