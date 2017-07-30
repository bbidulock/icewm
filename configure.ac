#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([icewm],[1.4.2],[https://github.com/bbidulock/icewm/issues])
AC_REVISION([1.4.2])
AC_CONFIG_SRCDIR([src/ysmapp.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS
AC_CANONICAL_TARGET

DATE='2017-07-30'
AC_SUBST([DATE])

APPLICATIONS='icewm icewm-session icesh icewmhint icewmbg icewmtray'
TESTCASES=`echo src/test*.cc | sed 's%src/\([[^ ]]*\)\.cc%\1%g'`
TESTCASES="$TESTCASES iceview icesame iceicon icerun icelist icebar" # iceclock
features=''

# Initialize Automake
AM_INIT_AUTOMAKE([1.13.4 dist-xz no-dist-gzip std-options -Wall color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_PROG_MKDIR_P
AC_ARG_VAR([LD], [Linker loader command])

AC_ARG_VAR([MKFONTDIR],[mkfontdir command. @<:@default=mkfontdir@:>@])
AC_CHECK_PROGS([MKFONTDIR],[mkfontdir],[${am_missing_run}mkfontdir],[$PATH$PATH_SEPARATOR/usr/X11R6/bin])

AC_ARG_VAR([MARKDOWN],[markdown command. @<:@default=markdown@:>@])
AC_CHECK_PROGS([MARKDOWN],[markdown],[${am_missing_run}markdown],[$PATH])

AC_ARG_VAR([ASCIIDOC],[asciidoc command. @<:@default=asciidoctor@:>@])
AC_CHECK_PROGS([ASCIIDOC],[asciidoctor asciidoc],[${am_missing_run}asciidoctor],[$PATH])

AC_ARG_VAR([ASCIIDOC_BACKEND],[asciidoc backend @<:@default=NONE@:>@])
if test -z "$ASCIIDOC_BACKEND"; then
	case "$ASCIIDOC" in
		asciidoc) ASCIIDOC_BACKEND="--backend=html4" ;;
		asciidoctor) ASCIIDOC_BACKEND="--backend=html5" ;;
	esac
fi

LT_PATH_LD
LT_PREREQ([2.4.2])
LT_INIT([dlopen])
LT_LANG([C++])
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

AM_GNU_GETTEXT_VERSION([0.19.2])
AM_GNU_GETTEXT([external])

AC_ARG_ENABLE([i18n],
    AC_HELP_STRING([--disable-i18n],[Disable internationalization.]))
if test x$enable_i18n != xno; then
    AC_DEFINE([CONFIG_I18N], [1], [Define to enable internationalization.])
    features="$features i18n"
fi

AC_ARG_WITH([unicode-set],
    AC_HELP_STRING([--with-unicode-set=CODESET],[Your iconv unicode set in
	machine endian encoding (e.g. WCHAR_T, UCS-4-INTERNAL, UCS-4LE,
	UCS-4BE)]),
    [AC_DEFINE_UNQUOTED([CONFIG_UNICODE_SET],"$with_unicode_set",[Preferred unicode set.])])

PKG_PROG_PKG_CONFIG

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_PATH_X
AC_PATH_XTRA
AC_CHECK_HEADERS([fcntl.h langinfo.h libintl.h limits.h locale.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h wchar.h])
AC_CHECK_HEADERS([linux/threads.h linux/tasks.h])
AC_CHECK_HEADERS([sched.h sys/dkstat.h sys/param.h sys/sysctl.h uvm/uvm_param.h])
AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([machine/apmvar.h])
AC_CHECK_HEADERS([machine/apm_bios.h])
AC_CHECK_HEADERS([execinfo.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)

AC_TYPE_SIGNAL

# Checks for library functions.
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_FUNC_ERROR_AT_LINE
if test x$cross_compling != xyes ; then :; AC_FUNC_FORK fi
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
if test x$cross_compling != xyes ; then :; AC_FUNC_MALLOC fi
AC_FUNC_MMAP
if test x$cross_compling != xyes ; then :; AC_FUNC_REALLOC fi
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([dup2 getcwd gethostbyname gethostname gettimeofday mblen memchr memmove memset mkdir nl_langinfo putenv select setlocale socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strstr strtol sysinfo uname abort wordexp])
AC_CHECK_FUNCS([gettimeofday putenv select socket strtol strtoul basename])
AC_CHECK_FUNCS([sysctlbyname asprintf])
AC_CHECK_FUNC([getloadavg],[AC_DEFINE([HAVE_GETLOADAVG2], 1, [getloadavg() is available])])
AC_FUNC_SELECT_ARGTYPES

PKG_CHECK_MODULES([CORE],[fontconfig xext x11])
AC_SUBST([CORE_CFLAGS])
AC_SUBST([CORE_LIBS])

AC_CHECK_LIB([X11],[XInternAtoms],
    [AC_DEFINE([HAVE_XINTERNATOMS],[1],[Define to enable XInternAtoms])])

AC_ARG_ENABLE([sm],
    AC_HELP_STRING([--disable-sm],[Disable X session management.]))
if test x$enable_sm != xno ; then
    PKG_CHECK_MODULES([XSM],[sm ice],[
	AC_DEFINE([CONFIG_SESSION],[1],[Define to enable X session management])],
	[AC_MSG_WARN([XSM not supported.])])
fi

AC_ARG_ENABLE([shape],
    AC_HELP_STRING([--disable-shape],[Disable use of SHAPE extension.]))
AC_ARG_ENABLE([shaped-decorations],
    AC_HELP_STRING([--disable-shaped-decorations], [Disable transparent frame
	decoration (titlebar borders), requests X shape extension (experimental).]))

if test x$enable_shaped_decorations = xyes ; then
    enable_shape=yes
fi
if test x$enable_shape != xno ; then
    AC_CHECK_LIB([Xext],[XShapeCombineRectangles],
	[AC_DEFINE([CONFIG_SHAPE],[1],[Define to enable X shape extension])
	if test x$enable_shaped_decorations != xno ; then
	    AC_DEFINE([CONFIG_SHAPED_DECORATION],[1],[Define to allow transparent frame borders.])
	    features="$features shaped-decorations"
	fi],[
	AC_MSG_WARN([SHAPE not supported.])
	if test x$enable_shaped_decorations = xyes ; then
	    AC_MSG_WARN([Shaped decorations not supported.])
	fi])
fi

AC_ARG_ENABLE([gradients],
    AC_HELP_STRING([--disable-gradients],[Disable gradients.]))
if test x$enable_gradients != xno; then
    AC_DEFINE([CONFIG_GRADIENTS],[1],[Define to enable gradient support.])
    features="$features gradients"
    enable_antialiasing=implied
fi

AC_ARG_ENABLE([xrandr],
    AC_HELP_STRING([--disable-xrandr],[Disable use of XRANDR extension.]))
if test x$enable_xrandr != xno; then
    PKG_CHECK_MODULES([XRANDR],[xrandr],[
	CORE_CFLAGS="$XRANDR_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$XRANDR_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_XRANDR],[1],[Define to enable XRANDR extension.])
	features="$features xrandr"],
	[AC_MSG_WARN([XRANDR not supported.])])
fi

AC_ARG_ENABLE([xfreetype],
    AC_HELP_STRING([--disable-xfreetype],[Disable use of XFT for text rendering.]))
if test x$enable_xfreetype != xno; then
    PKG_CHECK_MODULES([XFT2],[xft >= 2.0.0],[
	CORE_CFLAGS="$XFT2_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$XFT2_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_XFREETYPE],[2],[Define to enable XFT support.])
	features="$features xfreetype"],[
	    enable_corefonts=yes
	    PKG_CHECK_MODULES([XFT],[xft],[
		CORE_CFLAGS="$XFT_CFLAGS $CORE_CFLAGS"
		CORE_LIBS="$XFT_LIBS $CORE_LIBS"
		AC_DEFINE([CONFIG_XFREETYPE],[1])
		features="$features xfreetype"],[
		    AC_MSG_WARN([XFT not supported.])
		    enable_xfreetype=no])])
else
    enable_corefonts=yes
fi

AC_ARG_ENABLE([corefonts],
    AC_HELP_STRING([--enable-corefonts],[Support X11 core fonts.]))
if test x$enable_corefonts = xyes; then
    AC_DEFINE([CONFIG_COREFONTS],[1],[Define to enable X11 core fonts.])
    features="$features corefonts"
fi

AC_ARG_ENABLE([antialiasing],
    AC_HELP_STRING([--disable-antialiasing],[Disable antialiasing support.]))
if test x$enable_xfreetype != xno -a x$enable_antialiasing != xno; then
    AC_DEFINE([CONFIG_ANTIALIASING],[1],[Define to enable antialiasing.])
    features="$features antialiasing"
else
    if test x$enable_antialiasing = xyes; then
	AC_MSG_WARN([Antialiasing not supported.])
	enable_antialiasing=no
    fi
fi

AC_ARG_ENABLE([pda],
    AC_HELP_STRING([--enable-pda],[Enable PDA operation (disables window hiding).]))
if test x$enable_pda = xyes; then
    AC_DEFINE([CONFIG_PDA],[1],[Define to enable PDA operation (no window hiding).])
    features="${features} pda"
fi

AC_ARG_ENABLE([guievents],
    AC_HELP_STRING([--enable-guievents],[Enable GUI events (experimental).]))
AC_ARG_WITH([icesound],
    AC_HELP_STRING([--with-icesound=INTERFACES],[List of audio interfaces for icesound.
	Requires support for GUI events.  @<:@default: ALSA,OSS,Y,ESound@:>@]),[],
    [with_icesound='ALSA,OSS,Y,ESound'])
if test x$enable_guievents = xyes ; then
    AC_DEFINE([CONFIG_GUIEVENTS],[1],[Define to enable GUI events support.])
    features="${features} gui-events"

    for iface in `echo "$with_icesound"|sed 's|,| |g'` ; do
	case $iface in
	(ALSA|alsa)
	    PKG_CHECK_MODULES([ALSA],[sndfile alsa],[
		AUDIO_CFLAGS="$AUDIO_CFLAGS $ALSA_CFLAGS"
		AUDIO_LIBS="$AUDIO_LIBS $ALSA_LIBS"
		AC_DEFINE([ENABLE_ALSA],[1],[Define to enable ALSA support.])
		CONFIG_ALSA=yes],[AC_MSG_WARN([ALSA is not supported])])
	    ;;
	(OSS|oss)
	    AC_DEFINE([ENABLE_OSS],[1],[Define to enable OSS support.])
	    CONFIG_OSS=yes
	    ;;
	(Y|Y2|YIFF|y|y2|yiff)
	    PKG_CHECK_MODULES([YIFF],[Y2],[
		AUDIO_CFLAGS="$AUDIO_CFLAGS $YIFF_CFLAGS"
		AUDIO_LIBS="$AUDIO_LIBS $YIFF_LIBS"
		AC_DEFINE([ENABLE_YIFF],[1],[Define to enable YIFF support.])
		CONFIG_YIFF=yes],[AC_MSG_WARN([YIFF is not supported])])
	    ;;
	(ESound|ESD|esound|esd)
	    PKG_CHECK_MODULES([ESD],[esound],[
		AUDIO_CFLAGS="$AUDIO_CFLAGS $ESD_CFLAGS"
		AUDIO_LIBS="$AUDIO_LIBS $ESD_LIBS"
		AC_DEFINE([ENABLE_ESD],[1],[Define to enable ESD support.])
		CONFIG_ESD=yes],[AC_MSG_WARN([ESD is not supported])])
	    ;;
	(*)
	    AC_MSG_WARN([Invalid audio interface: $iface])
	    ;;
	esac
    done
    for iface in ALSA OSS YIFF ESD; do
	eval "test \"\$CONFIG_$iface\" = yes" && audio_support="$audio_support $iface"
    done
    if test "x$audio_support" != x ; then
	BUILD_SOUND=yes
	APPLICATIONS="${APPLICATIONS} icesound"
    else
	AC_MSG_WARN([No valid sound interface, not building icesound.])
    fi
fi

AC_SUBST([AUDIO_CFLAGS])
AC_SUBST([AUDIO_LIBS])

AC_ARG_ENABLE([xinerama],
    AC_HELP_STRING([--disable-xinerama],[Disable XINERAMA support.]))
if test x$enable_xinerama != xno ; then
    PKG_CHECK_MODULES([XINERAMA],[xinerama],[
	CORE_CFLAGS="$XINERAMA_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$XINERAMA_LIBS $CORE_LIBS"
	AC_DEFINE([XINERAMA],[1],[Define to enable Xinerama support.])
	features="$features xinerama"],
	[AC_MSG_WARN([XINERAMA is not supported.])])
fi

AC_ARG_ENABLE([fribidi],
    AC_HELP_STRING([--disable-fribidi],[Disable right to left support.]))
if test "$enable_fribidi" != "no" && test "$enable_i18n" != "no"; then
    PKG_CHECK_MODULES([FRIBIDI], [fribidi],[
	CORE_CFLAGS="$FRIBIDI_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$FRIBIDI_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_FRIBIDI],[1],[Define to enable fribidi support.])
	features="$features fribidi"],
	[AC_MSG_WARN([FRIBIDI is not supported.])])
fi

AC_ARG_ENABLE([prefs],
    AC_HELP_STRING([--disable-prefs],[Disable configurable preferences.]))
if test x$enable_prefs = xno ; then
    AC_DEFINE([NO_CONFIGURE],[1],[Define to disable preferences support.])
fi

AC_ARG_ENABLE([keyconf],
    AC_HELP_STRING([--disable-keyconf],[Disable configurable key bindings.]))
if test x$enable_keyconf = xno ; then
    AC_DEFINE([NO_KEYBIND],[1],[Define to disable keybinding support.])
fi

AC_ARG_ENABLE([menuconf],
    AC_HELP_STRING([--disable-menuconf],[Disable configurable menus.]))
if test x$enable_menuconf = xno ; then
    AC_DEFINE([NO_CONFIGURE_MENUS],[1],[Define to disable configurable menu support.])
fi

AC_ARG_ENABLE([winoptions],
    AC_HELP_STRING([--disable-winoptions],[Disable configurable window options.]))
if test x$enable_winoptions = xno ; then
    AC_DEFINE([NO_WINDOW_OPTIONS],[1],[Define to disable configurable window options support.])
fi

AC_ARG_ENABLE([taskbar],
    AC_HELP_STRING([--disable-taskbar],[Disable builtin taskbar.]))
AC_ARG_ENABLE([winmenu],
    AC_HELP_STRING([--disable-winmenu],[Disable window list menu.]))
AC_ARG_ENABLE([lite],
    AC_HELP_STRING([--enable-lite],[Build lightweight version of IceWM.]))
if test x$enable_lite != xyes ; then
    AC_DEFINE([CONFIG_TOOLTIP],[1],[Define to support tooltips.])
    if test x$enable_taskbar != xno ; then
	AC_DEFINE([CONFIG_TASKBAR],[1],[Define to enable builtin taskbar.])
	AC_DEFINE([CONFIG_ADDRESSBAR],[1],[Define for address bar.])
	AC_DEFINE([CONFIG_TRAY],[1],[Define for system tray.])
	AC_DEFINE([CONFIG_APPLET_MAILBOX],[1],[Define for mailbox applet.])
	AC_DEFINE([CONFIG_APPLET_CPU_STATUS],[1],[Define for CPU status applet.])
	AC_DEFINE([CONFIG_APPLET_MEM_STATUS],[1],[Define for memory status applet.])
	AC_DEFINE([CONFIG_APPLET_NET_STATUS],[1],[Define for network status applet.])
	AC_DEFINE([CONFIG_APPLET_CLOCK],[1],[Define for LCD clock applet.])
	AC_DEFINE([CONFIG_APPLET_APM],[1],[Define for APM status applet.])
    fi
    AC_DEFINE([CONFIG_WINLIST],[1],[Define for OS/2 like window list.])
    if test x$enable_winmenu != xno ; then
	AC_DEFINE([CONFIG_WINMENU],[1],[Define to enable window list menu.])
    fi
    BUILD_HELP=yes
    APPLICATIONS="${APPLICATIONS} icehelp"
else
    AC_DEFINE([LITE],[1],[Lite version])
fi

AC_SUBST([GNOME1_CFLAGS])
AC_SUBST([GNOME1_LIBS])

AC_ARG_ENABLE([menus-gnome2],
    AC_HELP_STRING([--disable-menus-gnome2],[Disable display of GNOME 2 menus.]))
if test x$enable_menus_gnome2 != xno; then
    PKG_CHECK_MODULES([GNOME2],[gnome-desktop-2.0 libgnomeui-2.0],[
	GNOME_VER=2
	AC_DEFINE([CONFIG_GNOME_MENUS],[1],[Define to make IceWM more GNOME-friendly])
	BUILD_MENU_GNOME2=yes
	APPLICATIONS="${APPLICATIONS} icewm-menu-gnome2"],
	[AC_MSG_WARN([GNOME2 menus not supported.])])
fi

AC_SUBST([GNOME_VER])
AC_SUBST([GNOME2_CFLAGS])
AC_SUBST([GNOME2_LIBS])

AC_ARG_ENABLE([menus-fdo],
    AC_HELP_STRING([--disable-menus-fdo],[Disable support of freedesktop.org style menus.]))
if test x$enable_menus_fdo != xno; then
PKG_CHECK_MODULES([GIO],[gio-2.0 gio-unix-2.0],[
	AC_DEFINE([CONFIG_FDO_MENUS],[1],[Define to support freedesktop.org style menus.])
	BUILD_MENU_FDO=yes
	APPLICATIONS="${APPLICATIONS} icewm-menu-fdo"],
	[AC_MSG_WARN([Freedesktop style menus not supported.])])
fi
AC_SUBST([GIO_CFLAGS])
AC_SUBST([GIO_LIBS])

PKG_CHECK_MODULES([IMAGE],[gdk-pixbuf-xlib-2.0],[
    AC_DEFINE([CONFIG_GDK_PIXBUF_XLIB],[1],
	[Define to use gdk_pixbuf_xlib for image rendering.])
    image_library=gdk_pixbuf_xlib])
AC_SUBST([IMAGE_CFLAGS])
AC_SUBST([IMAGE_LIBS])

AM_CONDITIONAL([BUILD_TESTS],[test x$BUILD_TESTS = xyes])
AM_CONDITIONAL([BUILD_HELP],[test x$BUILD_HELP = xyes])
AM_CONDITIONAL([BUILD_SOUND],[test x$BUILD_SOUND = xyes])
AM_CONDITIONAL([BUILD_MENU_GNOME1],[test x$BUILD_MENU_GNOME1 = xyes])
AM_CONDITIONAL([BUILD_MENU_GNOME2],[test x$BUILD_MENU_GNOME2 = xyes])
AM_CONDITIONAL([BUILD_MENU_FDO],[test x$BUILD_MENU_FDO = xyes])

test "x$prefix" = xNONE && prefix="$ac_default_prefix"

AC_ARG_WITH([libdir],
    AC_HELP_STRING([--with-libdir=PATH],[Package data directory.
	@<:@default=DATADIR/icewm@:>@]))
if test -z "$with_libdir" -o "x$with_libdir" = xyes ; then
    with_libdir='${datadir}/icewm'
fi
ICE_EXPAND([LIBDIR],[${with_libdir}])
AC_DEFINE_UNQUOTED([LIBDIR],["${LIBDIR}"],
	[Define to package data directory.])

AC_ARG_WITH([cfgdir],
    AC_HELP_STRING([--with-cfgdir=PATH],[Package configuration directory.
	@<:@default=SYSCONFDIR/icewm@:>@]))
if test -z "$with_cfgdir" -o "x$with_cfgdir" = xyes ; then
    with_cfgdir='${sysconfdir}/icewm'
fi
ICE_EXPAND([CFGDIR],[${with_cfgdir}])
AC_DEFINE_UNQUOTED([CFGDIR],["${CFGDIR}"],
	[Define to package configuration directory.])

AC_ARG_WITH([docdir],
    AC_HELP_STRING([--with-docdir=PATH],[Package documentation directory.
	@<:@default=DOCDIR@:>@]))
if test -z "$with_docdir" -o "x$with_docdir" = xyes ; then
    with_docdir='${docdir}'
fi
ICE_EXPAND([DOCDIR],[${with_docdir}])
AC_DEFINE_UNQUOTED([DOCDIR],["${DOCDIR}"],
	[Define to package documentation directory.])

ICE_EXPAND([LOCDIR],[${localedir}])
AC_DEFINE_UNQUOTED([LOCDIR],["${LOCDIR}"],
	[Define to locale directory.])

AC_DEFINE_UNQUOTED([HOSTOS],["${host_os}"],
	[Define to the host operating system.])
AC_DEFINE_UNQUOTED([HOSTCPU],["${host_cpu}"],
	[Define to the host CPU.])
AC_DEFINE_UNQUOTED([ICEHELPIDX],["${DOCDIR}/icewm.html"],
	[Define to the IceWM help index file.])

AC_ARG_ENABLE([wmspec],
    AC_HELP_STRING([--disable-wmspec],[Disable NetWM/EWMH support.]))
if test x$enable_wmspec != xno ; then
    AC_DEFINE([WMSPEC_HINTS],[1],[Define to enable NetWM/EWMH support.])
fi

AC_ARG_ENABLE([gnome1],
    AC_HELP_STRING([--disable-gnome1],[Disable WinWM/WMH (GNOME1) support.]))
if test x$enable_gnome1 != xno ; then
    AC_DEFINE([GNOME1_HINTS],[1],[Define to enable WinWM/WMH (GNOME1) support.])
fi

AC_ARG_ENABLE([debug],
    AC_HELP_STRING([--enable-debug],[Enable debugging of IceWM.]))
if test x$enable_debug = xyes ; then
    AC_DEFINE([DEBUG],[1],[Define if you want to debug IceWM.])
    CXXFLAGS="$CXXFLAGS -g -DDEBUG -O0"
    features="$features debugging-symbols"
fi

CORE_LIBS="$CORE_LIBS -lm"

AC_ARG_VAR([EXTRA_LIBS], [Extra core libraries.])

CORE_LIBS="$CORE_LIBS "'$(EXTRA_LIBS)'

# needed by lib/menu.in
AC_SUBST([CONFIG_GNOME2_MENU_DIR])
dnl AC_SUBST([GNOME_VER])
AC_SUBST([CONFIG_KDE_MENU_DIR])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 po/Makefile.in
                 doc/Makefile
                 lib/Makefile
		 lib/keys
		 lib/menu
		 lib/programs
		 lib/toolbar
		 lib/winoptions])
AC_OUTPUT

# Display results
AC_MSG_RESULT([])
ICE_MSG_VALUE([Applications],	APPLICATIONS)
ICE_MSG_VALUE([Image library],	image_library)
ICE_MSG_VALUE([Audio support],	audio_support)
ICE_MSG_VALUE([Features],	features)
ICE_MSG_VALUE([Paths: PREFIX], prefix)
ICE_MSG_VALUE([       BINDIR], bindir)
ICE_MSG_VALUE([       LOCDIR], localedir)
ICE_MSG_VALUE([       LIBDIR], LIBDIR)
ICE_MSG_VALUE([       CFGDIR], CFGDIR)
ICE_MSG_VALUE([       KDEDIR], kdedatadir)
ICE_MSG_VALUE([       DOCDIR], DOCDIR)
ICE_MSG_VALUE([       MANDIR], mandir)
AC_MSG_RESULT([])
