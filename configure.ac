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

BRANCH='1.4.2'
AC_SUBST([BRANCH])

APPLICATIONS='icewm icewm-session icesh icewmhint icewmbg icewmtray icehelp'
TESTCASES=`echo src/test*.cc | sed 's%src/\([[^ ]]*\)\.cc%\1%g'`
TESTCASES="$TESTCASES iceview icesame iceicon icerun icelist icebar" # iceclock
features=''

# Initialize Automake
AM_INIT_AUTOMAKE([gnits 1.13.4 dist-xz no-dist-gzip std-options -Wall color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])
AC_PREFIX_DEFAULT([/usr])

# Checks for programs.
AC_LANG_CPLUSPLUS
AC_PROG_SED
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
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
		*) AC_MSG_WARN([ASCIIDOC $ASCIIDOC is not supported.]) ;;
	esac
fi

AC_ARG_VAR([LINKS],[links command. @<:@default=lynx@:>@])
AC_CHECK_PROGS([LINKS],[lynx links],[${am_missing_run}lynx],[$PATH])
if test "$LINKS" = "links"; then
	LINKS="links -codepage utf-8"
fi

AC_DISABLE_STATIC
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
AC_CHECK_HEADERS([execinfo.h fcntl.h])
AC_CHECK_HEADERS([langinfo.h libgen.h libintl.h limits.h locale.h])
AC_CHECK_HEADERS([machine/apm_bios.h machine/apmvar.h])
AC_CHECK_HEADERS([netdb.h netinet/in.h])
AC_CHECK_HEADERS([sched.h sndfile.h stddef.h stdlib.h string.h])
AC_CHECK_HEADERS([sys/dkstat.h sys/file.h sys/ioctl.h sys/param.h])
AC_CHECK_HEADERS([sys/sched.h sys/socket.h sys/soundcard.h sys/sysctl.h sys/time.h])
AC_CHECK_HEADERS([unistd.h uvm/uvm_param.h wchar.h])

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
if test x$cross_compling != xyes ; then :; AC_FUNC_FORK fi
if test x$cross_compling != xyes ; then :; AC_FUNC_MALLOC fi
if test x$cross_compling != xyes ; then :; AC_FUNC_REALLOC fi
AC_CHECK_FUNCS([abort backtrace_symbols_fd basename dup2])
AC_CHECK_FUNCS([gethostbyname gethostname gettimeofday])
AC_CHECK_FUNCS([if_nametoindex if_indextoname if_nameindex])
AC_CHECK_FUNCS([mblen memchr memmove memset mkdir nl_langinfo])
AC_CHECK_FUNCS([select setlocale socket])
AC_CHECK_FUNCS([strcasecmp strchr strcspn strdup strerror strncasecmp])
AC_CHECK_FUNCS([strrchr strsignal strspn strstr strtol strtoul])
AC_CHECK_FUNCS([sysctl sysctlbyname sysinfo uname wordexp])
AC_CHECK_FUNC([getloadavg],[AC_DEFINE([HAVE_GETLOADAVG2], 1, [getloadavg() is available])])
AC_FUNC_SELECT_ARGTYPES

AC_MSG_CHECKING([for strlcpy])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]],[[
               char a[2];
               if (strlcpy(a, "yes", 2) != 3)
                   return 1;]])],
              [
               AC_MSG_RESULT([yes])
               AC_DEFINE([HAVE_STRLCPY], 1,
                         [Define to 1 if you have the strlcpy function.])
              ],
              [
               AC_MSG_RESULT([no])
               AH_TEMPLATE([HAVE_STRLCPY],
                           [Define to 1 if you have the strlcpy function.])
              ])

AC_MSG_CHECKING([for strlcat])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]],[[
               char a[4] = "no";
               if (strlcat(a, "yes", 4) != 5)
                   return 1;]])],
              [
               AC_MSG_RESULT([yes])
               AC_DEFINE([HAVE_STRLCAT], 1,
                         [Define to 1 if you have the strlcat function.])
              ],
              [
               AC_MSG_RESULT([no])
               AH_TEMPLATE([HAVE_STRLCAT],
                           [Define to 1 if you have the strlcat function.])
              ])


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
if test x$enable_shape != xno ; then
    PKG_CHECK_MODULES([SHAPE],[xext],[
	CORE_CFLAGS="$SHAPE_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$SHAPE_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_SHAPE],[1],[Define to enable X shape extension])
	features="$features shape"],
	[AC_MSG_WARN([SHAPE not supported.])])
else
    AC_MSG_WARN([SHAPE disabled.])
fi

AC_ARG_ENABLE([xrender],
    AC_HELP_STRING([--disable-xrender],[Disable use of RENDER extension.]))
if test x$enable_xrender != xno ; then
    PKG_CHECK_MODULES([RENDER],[xrender],[
	CORE_CFLAGS="$RENDER_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$RENDER_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_RENDER],[1],[Define to enable RENDER extension.])
	features="$features render"],
	[AC_MSG_WARN([RENDER not supported.])])
else
    AC_MSG_WARN([RENDER disabled.])
fi

AC_ARG_ENABLE([xrandr],
    AC_HELP_STRING([--disable-xrandr],[Disable use of RANDR extension.]))
if test x$enable_xrandr != xno; then
    PKG_CHECK_MODULES([RANDR],[xrandr],[
	CORE_CFLAGS="$RANDR_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$RANDR_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_XRANDR],[1],[Define to enable RANDR extension.])
	features="$features xrandr"],
	[AC_MSG_WARN([RANDR not supported.])])
else
    AC_MSG_WARN([RANDR disabled.])
fi

AC_ARG_ENABLE([xfreetype],
    AC_HELP_STRING([--disable-xfreetype],[Disable use of XFT for text rendering.]))
if test x$enable_xfreetype != xno; then
    PKG_CHECK_MODULES([XFT2],[xft >= 2.0.0],[
	CORE_CFLAGS="$XFT2_CFLAGS $CORE_CFLAGS"
	CORE_LIBS="$XFT2_LIBS $CORE_LIBS"
	AC_DEFINE([CONFIG_XFREETYPE],[2],[Define to enable XFT support.])
	features="$features xfreetype"],
	[AC_MSG_WARN([XFT 2+ not supported.])])
else
    enable_corefonts=yes
fi

AC_ARG_ENABLE([corefonts],
    AC_HELP_STRING([--enable-corefonts],[Support X11 core fonts.]))
if test x$enable_corefonts = xyes; then
    AC_DEFINE([CONFIG_COREFONTS],[1],[Define to enable X11 core fonts.])
    features="$features corefonts"
fi

AC_ARG_WITH([icesound],
    AC_HELP_STRING([--with-icesound=INTERFACES],[List of audio interfaces for icesound. @<:@default: ALSA,AO,OSS,ESD@:>@]),[],
    [with_icesound='ALSA,AO,OSS,ESD'])

for iface in `echo "$with_icesound"|sed 's|,| |g'` ; do
    case $iface in
    (ALSA|alsa)
        PKG_CHECK_MODULES([ALSA],[sndfile alsa],[
            AUDIO_CFLAGS="$AUDIO_CFLAGS $ALSA_CFLAGS"
            AUDIO_LIBS="$AUDIO_LIBS $ALSA_LIBS"
            AC_DEFINE([ENABLE_ALSA],[1],[Define to enable ALSA support.])
            CONFIG_ALSA=yes],[AC_MSG_WARN([ALSA is not supported])])
        ;;
    (AO|ao)
        PKG_CHECK_MODULES([AO],[sndfile ao],[
            AUDIO_CFLAGS="$AUDIO_CFLAGS $AO_CFLAGS"
            AUDIO_LIBS="$AUDIO_LIBS $AO_LIBS"
            AC_DEFINE([ENABLE_AO],[1],[Define to enable AO support.])
            CONFIG_AO=yes],[AC_MSG_WARN([AO is not supported])])
        ;;
    (OSS|oss)
        PKG_CHECK_MODULES([OSS],[sndfile],[
            AUDIO_CFLAGS="$AUDIO_CFLAGS $OSS_CFLAGS"
            AUDIO_LIBS="$AUDIO_LIBS $OSS_LIBS"
            AC_DEFINE([ENABLE_OSS],[1],[Define to enable OSS support.])
            CONFIG_OSS=yes],[AC_MSG_WARN([OSS is not supported])])
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
for iface in ALSA AO OSS ESD; do
    eval "test \"\$CONFIG_$iface\" = yes" && audio_support="$audio_support $iface"
done
if test "x$audio_support" != x ; then
    BUILD_SOUND=yes
    APPLICATIONS="${APPLICATIONS} icesound"
else
    AC_MSG_WARN([No valid sound interface, not building icesound.])
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

AC_ARG_ENABLE([gdk-pixbuf],
    AC_HELP_STRING([--enable-gdk-pixbuf],[Enable gdk-pixbuf rendering.
	@<:@default=disabled@:>@]))
if test "x$enable_gdk_pixbuf" = xyes ; then
    PKG_CHECK_MODULES([PIXBUF],[gdk-pixbuf-xlib-2.0],[
	IMAGE_CFLAGS="$PIXBUF_CFLAGS"
	IMAGE_LIBS="$PIXBUF_LIBS"
	AC_DEFINE([CONFIG_GDK_PIXBUF_XLIB],[1],[Define to enable gdk-pixbuf rendering.])
	image_library=gdk-pixbuf-xlib-2.0])
    PKG_CHECK_MODULES([LIBRSVG],[librsvg-2.0],[
	IMAGE_CFLAGS="$LIBRSVG_CFLAGS $IMAGE_CFLAGS"
	IMAG_LIBS="$LIBRSVG_LIBS $IMAGE_LIBS"
	AC_DEFINE([CONFIG_LIBRSVG],[1],[Define to enable librsvg rendering.])
	image_library="$image_library librsvg-2.0"])
else
    PKG_CHECK_MODULES([XPM],[xpm],[
	IMAGE_CFLAGS="$XPM_CFLAGS"
	IMAGE_LIBS="$XPM_LIBS"
	AC_DEFINE([CONFIG_XPM],[1],[Define to enable libxpm rendering.])
	image_library=libxpm])
    PKG_CHECK_MODULES([LIBPNG],[libpng >= 1.2],[
	IMAGE_CFLAGS="$LIBPNG_CFLAGS $IMAGE_CFLAGS"
	IMAGE_LIBS="$LIBPNG_LIBS $IMAGE_LIBS"
	AC_DEFINE([CONFIG_LIBPNG],[1],[Define to enable libpng rendering.])
	image_library="$image_library libpng"])
    PKG_CHECK_MODULES([LIBJPEG],[libjpeg],[
        IMAGE_CFLAGS="$LIBJPEG_CFLAGS $IMAGE_CFLAGS"
        IMAGE_LIBS="$LIBJPEG_LIBS $IMAGE_LIBS"
        AC_DEFINE([CONFIG_LIBJPEG],[1],[Define to enable libjpeg rendering.])
        image_library="$image_library libjpeg"],
        [if test -f /usr/include/jpeglib.h &&
            test -x /usr/lib64/libjpeg.so ; then
            IMAGE_LIBS="-ljpeg $IMAGE_LIBS"
            AC_DEFINE([CONFIG_LIBJPEG],[1],[Define to enable libjpeg rendering.])
        fi])
fi
AC_SUBST([IMAGE_CFLAGS])
AC_SUBST([IMAGE_LIBS])

AM_CONDITIONAL([BUILD_TESTS],[test x$BUILD_TESTS = xyes])
AM_CONDITIONAL([BUILD_SOUND],[test x$BUILD_SOUND = xyes])
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

AC_ARG_WITH([theme],
    AC_HELP_STRING([--with-theme=NAME],[Default theme.
                    [default="default/default.theme"]]),,
                    with_theme="default/default.theme")
AC_DEFINE_UNQUOTED([CONFIG_DEFAULT_THEME],
                   "${with_theme}",[Name of default theme])

AC_DEFINE_UNQUOTED([HOSTOS],["${host_os}"],
	[Define to the host operating system.])
AC_DEFINE_UNQUOTED([HOSTCPU],["${host_cpu}"],
	[Define to the host CPU.])
AC_DEFINE_UNQUOTED([ICEHELPIDX],["${DOCDIR}/icewm.html"],
	[Define to the IceWM help index file.])

AC_ARG_ENABLE([debug],
    AC_HELP_STRING([--enable-debug],[Enable debugging of IceWM.]))
if test x$enable_debug = xyes ; then
    AC_DEFINE([DEBUG],[1],[Define if you want to debug IceWM.])
    CXXFLAGS="$CXXFLAGS -g -DDEBUG"
    features="$features debugging-symbols"
fi
AC_ARG_ENABLE([logevents],
    AC_HELP_STRING([--enable-logevents],[Support logging of X11 events.]))
if test x$enable_logevents = xyes ; then
    AC_DEFINE([LOGEVENTS],[1],[Define to enable X11 event logging support.])
    features="$features logevents"
fi

CORE_LIBS="$CORE_LIBS -lm"

AC_ARG_VAR([EXTRA_LIBS], [Extra core libraries.])

CORE_LIBS="$CORE_LIBS "'$(EXTRA_LIBS)'

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 po/Makefile.in
                 doc/Makefile
		 man/Makefile
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

# vim: set ft=config sw=4:
