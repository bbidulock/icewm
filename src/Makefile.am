
########
########    WARNING: DO NOT CHANGE THIS FILE WITHOUT RUNNING dist.sh SUCCESSFULLY
########    BEFORE PUSHING YOUR CHANGES!
########


AM_CPPFLAGS = -include ../config.h
AM_CXXFLAGS = $(CORE_CFLAGS) $(XSM_CFLAGS) $(IMAGE_CFLAGS) $(AUDIO_CFLAGS) $(GIO_CFLAGS) -DEXEEXT=$(EXEEXT)

EXTRA_DIST = \
	ypointer.h \
	globit.c

EXTRA_PROGRAMS = \
	icehelp \
	icesound \
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
	icehelp \
	icewmhint \
	icewmbg \
	icewmtray

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = icewmtray

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
	strtest
endif

if BUILD_SOUND
bin_PROGRAMS += icesound
endif

if BUILD_MENU_FDO
bin_PROGRAMS += icewm-menu-fdo
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
	ylist.h \
	ywindow.cc \
	ywindow.h \
	ycolor.cc \
	ycolor.h \
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
	ylocale.cc \
	ylocale.h \
	yarray.cc \
	yarray.h \
	ycollections.h \
	ycollections.cc \
	ypipereader.cc \
	ypipereader.h \
	yxembed.cc \
	yxembed.h \
	binascii.h \
	argument.h \
	yconfig.cc \
	yconfig.h \
	yprefs.cc \
	yprefs.h \
	yfont.cc \
	yfontcore.cc \
	yfontxft.cc \
	ypixmap.cc \
	ypixmap.h \
	yimage.h \
	yimage_gdk.cc \
	yximage.cc \
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
	wmconfig.h \
	wpixmaps.h \
	wpixres.cc \
	wpixres.h
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
genpref_LDADD = libice.la @LIBINTL@

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
	ylib.h \
	ymsgbox.cc \
	ymsgbox.h \
	ydialog.cc \
	ydialog.h \
	binascii.h \
	yurl.cc \
	yurl.h \
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
	wmaction.h \
	wmcontainer.cc \
	wmcontainer.h \
	wmclient.cc \
	MwmUtil.h \
	wmclient.h \
	wmmgr.cc \
	wmmgr.h \
	appnames.h \
	guievent.h \
	wmapp.cc \
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
	wmmenu.cc \
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
	applet.cc \
	applet.h \
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
	yxcontext.h \
	yxtray.cc \
	yxtray.h
icewm_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(XSM_LIBS) $(CORE_LIBS)

icesh_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	WinMgr.h \
	icesh.cc
icesh_LDADD = libice.la $(CORE_LIBS) @LIBINTL@

icewm_session_SOURCES = \
	appnames.h \
	base.h \
	debug.h \
	intl.h \
	sysdep.h \
	yapp.h \
	ytimer.h \
	icesm.cc
icewm_session_LDADD = libice.la @LIBINTL@

icewmhint_SOURCES = \
	intl.h \
	debug.h \
	base.h \
	icewmhint.cc
icewmhint_LDADD = libice.la $(CORE_LIBS) @LIBINTL@

icewmbg_SOURCES = \
	intl.h \
	debug.h \
	base.h \
	themable.h \
	default.h \
	ykey.h \
	yfull.h \
	yxapp.h \
	yprefs.h \
	ypaths.h \
	icewmbg.cc \
	icewmbg_prefs.h
icewmbg_LDADD = libice.la $(IMAGE_LIBS) $(CORE_LIBS) @LIBINTL@

icesound_SOURCES = \
	base.h \
	debug.h \
	guievent.h \
	intl.h \
	upath.h \
	ypointer.h \
	ytimer.h \
	icesound.cc
icesound_LDADD = libice.la $(AUDIO_LIBS) $(CORE_LIBS) @LIBINTL@

icewm_menu_fdo_SOURCES = \
	sysdep.h \
	base.h \
	fdospecgen.h \
	fdomenu.cc
icewm_menu_fdo_LDADD = libice.la $(GIO_LIBS)

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
	yrect.h \
	icehelp.cc
icehelp_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS) @LIBINTL@

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
iceclock_LDADD = libitk.la libice.la $(IMAGE_LIBS) $(CORE_LIBS) @LIBINTL@

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
	wmoption.cc \
	wmoption.h \
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
	wmmenu.cc \
	wmprog.cc \
	wmprog.h \
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
testlocale_LDADD = libice.la $(CORE_LIBS) @LIBINTL@

testarray_SOURCES = \
	intl.h \
	debug.h \
	sysdep.h \
	base.h \
	ylib.h \
	testarray.cc
testarray_LDADD = libice.la @LIBINTL@

nodist_pkgdata_DATA = \
	preferences

preferences: genpref$(EXEEXT)
	$(AM_V_GEN)./genpref$(EXEEXT) >$@

CLEANFILES = preferences

