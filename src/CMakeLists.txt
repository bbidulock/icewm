cmake_minimum_required(VERSION 2.6)

PROJECT(ICEWM CXX)

INCLUDE(CheckIncludeFiles) 
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckTypeSize)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(FindPkgConfig)
INCLUDE(CheckCXXSourceCompiles)

SET(CXXFLAGS_COMMON "-pthread -Wall -Wextra -Wno-unused-parameter -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -I.")

if("${CMAKE_BUILD_TYPE}" STREQUAL "" AND ("" STREQUAL "${CMAKE_CXX_FLAGS}"))
	SET(CMAKE_BUILD_TYPE Release)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
   add_definitions(-DDEBUG)
endif()

# I don't need -rdynamic, thanks!
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# apply to test calls
SET(CMAKE_REQUIRED_FLAGS ${CXXFLAGS_COMMON})

option(CONFIG_XPM "Legacy XPM image loader" off)
option(CONFIG_IMLIB "(Deprecated) IMLIB image loader" off)
option(CONFIG_GDK_PIXBUF_XLIB "LIBGDKPIXBUF based image loader" on)
option(CONFIG_I18N "Define to enable internationalization" on)
option(ENABLE_NLS "Enable Native Language Support" on)
option(CONFIG_SESSION "Define to enable X session management" on)

SET(CONFIG_UNICODE_SET "" CACHE STRING "Your iconv unicode set in machine endian encoding (e.g. WCHAR_T, UCS-4-INTERNAL, UCS-4LE, UCS-4BE)")
# turn off when empty, see config.h template for details
IF("" STREQUAL "${CONFIG_UNICODE_SET}")
	SET(CONFIG_UNICODE_SET OFF)
ELSE()
	MESSAGE(STATUS "Configured custom unicode set: ${CONFIG_UNICODE_SET}.")
ENDIF()

# TODO: find some common helper to handle that variables
if(ENABLE_NLS OR CONFIG_I18N)
	find_library(CONFIG_LIBICONV iconv)
	if(CONFIG_LIBICONV)
		LIST(APPEND nls_LIBS iconv)
	endif()
	find_library(CONFIG_LIBINTL intl)
	if(CONFIG_LIBINTL)
		LIST(APPEND nls_LIBS intl)
	endif()
endif()

# the only used ones...
# for x in `cat funclist` ; do grep $x src/* lib/* && echo $x >> exlist ; done 
# perl -e 'print "CHECK_FUNCTION_EXISTS($_ HAVE_".uc($_).")\n" for @ARGV' `cat exlist`
# perl -e 'print "#cmakedefine HAVE_".uc($_)."\n" for @ARGV' `cat exlist`
CHECK_FUNCTION_EXISTS(dup2 HAVE_DUP2)
CHECK_FUNCTION_EXISTS(getcwd HAVE_GETCWD)
CHECK_FUNCTION_EXISTS(gethostbyname HAVE_GETHOSTBYNAME)
CHECK_FUNCTION_EXISTS(gethostname HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(mblen HAVE_MBLEN)
CHECK_FUNCTION_EXISTS(memchr HAVE_MEMCHR)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(mkdir HAVE_MKDIR)
CHECK_FUNCTION_EXISTS(nl_langinfo HAVE_NL_LANGINFO)
CHECK_FUNCTION_EXISTS(putenv HAVE_PUTENV)
CHECK_FUNCTION_EXISTS(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strcspn HAVE_STRCSPN)
CHECK_FUNCTION_EXISTS(strdup HAVE_STRDUP)
CHECK_FUNCTION_EXISTS(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strncasecmp HAVE_STRNCASECMP)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strspn HAVE_STRSPN)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(sysinfo HAVE_SYSINFO)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)
CHECK_FUNCTION_EXISTS(abort HAVE_ABORT)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(putenv HAVE_PUTENV)
CHECK_FUNCTION_EXISTS(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(basename HAVE_BASENAME)
CHECK_FUNCTION_EXISTS(sysctlbyname HAVE_SYSCTLBYNAME)
CHECK_FUNCTION_EXISTS(getloadavg HAVE_GETLOADAVG)
CHECK_FUNCTION_EXISTS(asprintf HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS(wordexp HAVE_WORDEXP)

CHECK_LIBRARY_EXISTS (X11 XInternAtoms "" HAVE_XINTERNATOMS)

# perl -e 'do { s/\.|\//_/g; $_=uc $_;  print "#cmakedefine HAVE_$_\n"} for @ARGV' `cat headersDa`
# perl -e 'do {my $orig=$_; s/\.|\//_/g; $_=uc $_; print "CHECK_INCLUDE_FILE_CXX($orig HAVE_$_)\n"} for @ARGV' `cat headersDa`
CHECK_INCLUDE_FILE_CXX(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILE_CXX(langinfo.h HAVE_LANGINFO_H)
CHECK_INCLUDE_FILE_CXX(libintl.h HAVE_LIBINTL_H)
CHECK_INCLUDE_FILE_CXX(limits.h HAVE_LIMITS_H)
CHECK_INCLUDE_FILE_CXX(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILE_CXX(netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILE_CXX(stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILE_CXX(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_CXX(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE_CXX(sys/file.h HAVE_SYS_FILE_H)
CHECK_INCLUDE_FILE_CXX(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE_CXX(sys/param.h HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CXX(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CXX(wchar.h HAVE_WCHAR_H)
CHECK_INCLUDE_FILE_CXX(sched.h HAVE_SCHED_H)
CHECK_INCLUDE_FILE_CXX(sys/dkstat.h HAVE_SYS_DKSTAT_H)
CHECK_INCLUDE_FILE_CXX(sys/param.h HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE_CXX(sys/sysctl.h HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE_CXX(uvm/uvm_param.h HAVE_UVM_UVM_PARAM_H)
CHECK_INCLUDE_FILE_CXX(libgen.h HAVE_LIBGEN_H)
CHECK_INCLUDE_FILE_CXX(machine/apmvar.h HAVE_MACHINE_APMVAR_H)
CHECK_INCLUDE_FILE_CXX(machine/apm_bios.h HAVE_MACHINE_APM_BIOS_H)
CHECK_INCLUDE_FILE_CXX(execinfo.h HAVE_EXECINFO_H)

CHECK_TYPE_SIZE(char SIZEOF_CHAR)
CHECK_TYPE_SIZE(long SIZEOF_LONG)
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(short SIZEOF_SHORT)

#########################################################
# fiting flags to options and available system features #
#########################################################

pkg_check_modules(fontconfig fontconfig)
pkg_check_modules(xext xext)
pkg_check_modules(x11 x11)
if(CONFIG_GDK_PIXBUF_XLIB)
	pkg_check_modules(pixbuf gdk-pixbuf-xlib-2.0)
endif()

IF(CONFIG_SESSION)
	LIST(APPEND sm_LIBS SM)
	LIST(APPEND sm_LIBS ICE)
ENDIF()

option(CONFIG_SHAPE "Define to enable transparent frame	decoration (titlebar borders), requests X shape extension." on)
# TODO: check the symbol: AC_CHECK_LIB([Xext],[XShapeCombineRectangles],
if(CONFIG_SHAPE AND NOT xext_FOUND)
   message(WARNING "Xext library not found, disabling CONFIG_SHAPE")
   set(CONFIG_SHAPE off)
endif()
if(CONFIG_SHAPE)
	option(CONFIG_SHAPED_DECORATION "Define to allow transparent frame borders" on)
endif()

option(CONFIG_ANTIALIASING "CONFIG_ANTIALIASING" on)
option(CONFIG_GRADIENTS "CONFIG_GRADIENTS" on)
if(CONFIG_GRADIENTS)
   set(CONFIG_ANTIALIASING on)
endif()

option(CONFIG_XRANDR "Define to enable XRANDR extension" on)
if(CONFIG_XRANDR)
pkg_check_modules(xrandr xrandr)
IF(NOT xrandr_FOUND)
   message(WARNING "Xrandr library not found, disabling CONFIG_XRANDR")
   set(CONFIG_XRANDR off)
ENDIF()
endif()

option(CONFIG_XFREETYPE "Define to enable XFT support" on)
if(CONFIG_XFREETYPE)
   set(CONFIG_XFREETYPE_VALUE 2)
   pkg_check_modules(xft xft>=2.0)
   IF(NOT xft_FOUND)
      set(CONFIG_XFREETYPE_VALUE 1)
      pkg_check_modules(xft xft)
   ENDIF()
   IF(NOT xft_FOUND)
      message(WARNING "xft library not found, disabling CONFIG_xft")
      set(CONFIG_XFREETYPE off)
   ENDIF()
endif()

option(CONFIG_COREFONTS "Define to enable X11 core fonts" off)
option(CONFIG_PDA "Define to enable PDA operation (no window hiding)" off)
option(CONFIG_GUIEVENTS "Define to enable GUI events (experimental)" off)
set(ICESOUND "ALSA,OSS,Y,ESound" CACHE STRING "List of supported audio subsystems")
if(CONFIG_GUIEVENTS)
	string(TOLOWER ${ICESOUND} ICESOUND)
	message(STATUS "Checking audio subsystems from ${ICESOUND}")
	if("${ICESOUND}" MATCHES "alsa")
		pkg_check_modules(alsa sndfile alsa)
		set(ENABLE_ALSA "${alsa_FOUND}")
	endif()
	if("${ICESOUND}" MATCHES "esound|esd")
		pkg_check_modules(esd esound)
		set(ENABLE_ESD "${esd_FOUND}")
	endif()
	if("${ICESOUND}" MATCHES "y")
		pkg_check_modules(yiff y2)
		set(ENABLE_YIFF "${yiff_FOUND}")
	endif()
	if("${ICESOUND}" MATCHES "oss")
		set(CONFIG_OSS on)
	endif()
endif()

option(XINERAMA "Define to enable Xinerama support" on)
if(XINERAMA)
	pkg_check_modules(xinerama xinerama)
	if(NOT xinerama_FOUND)
		message(WARNING "XINERAMA is not supported")
		set(XINERAMA off)
	endif()
endif()

option(CONFIG_FRIBIDI on)
if(CONFIG_FRIBIDI)
	pkg_check_modules(fribidi fribidi)
	if(not fribidi_FOUND)
		message(WARNING "FRIBIDI is not supported")
		set(CONFIG_FRIBIDI off)
			
	endif()
endif()

option(NO_CONFIGURE "Define to disable preferences support")
option(NO_KEYBIND "Define to disable keybinding support")
option(NO_CONFIGURE_MENUS "Define to disable configurable menu support")
option(NO_WINDOW_OPTIONS "Define to disable configurable window options support")
option(LITE "Lite version (overrides certain options)")
if(NOT LITE)
	option(CONFIG_TOOLTIP "Define to support tooltips" on)
	option(CONFIG_WINLIST "Define for OS/2 like window list" on)
	option(CONFIG_TASKBAR "Define to enable builtin taskbar" on)
	if(CONFIG_TASKBAR)
		option(CONFIG_ADDRESSBAR "Define for address bar" on)
		option(CONFIG_TRAY "Define for system tray" on)
		option(CONFIG_APPLET_MAILBOX "Define for mailbox applet" on)
		option(CONFIG_APPLET_CPU_STATUS "Define for CPU status applet" on)
		option(CONFIG_APPLET_MEM_STATUS "Define for memory status applet" on)
		option(CONFIG_APPLET_NET_STATUS "Define for network status applet" on)
		option(CONFIG_APPLET_CLOCK "Define for LCD clock applet" on)
		option(CONFIG_APPLET_APM "Define for APM status applet" on)
	endif()
	option(CONFIG_WINMENU "Define to enable window list menu" on)
endif()

option(CONFIG_FDO_MENUS "Define to support freedesktop.org style menus" on)
if(CONFIG_FDO_MENUS)
	pkg_check_modules(gio gio-2.0 gio-unix-2.0)
	if(NOT gio_FOUND)
		set(CONFIG_FDO_MENUS off)
		message(WARNING "Freedesktop style menus not supported")
	endif()
endif()

option(WMSPEC_HINTS "Define to enable NetWM/EWMH support" on)
option(GNOME1_HINTS "Define to enable WinWM/WMH (GNOME1) support" on)
option(DEBUG "Define if you want to debug IceWM" off)


#######################################
# some extra sugar where available    #
#######################################

SET(CMAKE_REQUIRED_FLAGS ${CXXFLAGS_COMMON})
SET(LDFLAGS_ORIG ${CMAKE_EXE_LINKER_FLAGS})
SET(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS_ORIG} -Wl,--as-needed ")
CHECK_CXX_SOURCE_COMPILES("int main() {return 0;}" USE_WLASNEEDED)
IF(NOT USE_WLASNEEDED)
   SET(CMAKE_EXE_LINKER_FLAGS ${LDFLAGS_ORIG})
ENDIF()

# teach GCC7 to interpret our comments
SET(CMAKE_REQUIRED_FLAGS "${CXXFLAGS_COMMON} -Wimplicit-fallthrough=2")
CHECK_CXX_SOURCE_COMPILES("int main() {return 0;}" USE_FALLTHROUGH_WARNING)
IF(USE_FALLTHROUGH_WARNING)
        SET(CXXFLAGS_COMMON "${CXXFLAGS_COMMON} -Wimplicit-fallthrough=2")
ENDIF()

#######################################
# all checks done, save configuration #
#######################################

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.cmake.h.in" "${CMAKE_BINARY_DIR}/config.h")

#######################################
# declaring targets and build flags   #
#######################################

# pkg-config delivers lists, we need plain compiler options! :-(
foreach(pfx fontconfig xext x11 pixbuf xrandr xft alsa 
		esd yiff xinerama fribidi gio)
	string(REPLACE ";" " " ${pfx}_CFLAGS "${${pfx}_CFLAGS}")
	#message(STATUS "Normalizing argument lists: ${${pfx}_LDFLAGS} ; ${${pfx}_CFLAGS}")
endforeach()

SET(ICE_COMMON_SRCS mstring.cc udir.cc upath.cc yapp.cc yxapp.cc ytimer.cc
 ywindow.cc ypaint.cc ypopup.cc misc.cc ycursor.cc ysocket.cc ypaths.cc
 ypixbuf.cc ylocale.cc yarray.cc ypipereader.cc yxembed.cc yconfig.cc
 yprefs.cc yfont.cc ypixmap.cc yimage.cc
 yimage_gdk.cc yimage_imlib.cc yimage_xpm.cc ytooltip.cc)

if(CONFIG_XFREETYPE)
	list(APPEND ICE_COMMON_SRCS yfontxft.cc)
endif()
if(CONFIG_COREFONTS)
	list(APPEND ICE_COMMON_SRCS yfontcore.cc)
endif()

SET(ITK_SRCS ymenu.cc ylabel.cc yscrollview.cc ymenuitem.cc yscrollbar.cc ybutton.cc ylistbox.cc yinput.cc yicon.cc wmconfig.cc ref.cc
   ydialog.cc ymsgbox.cc)

SET(ICEWM_SRCS ${ICE_COMMON_SRCS} ${ITK_SRCS}
 wmbutton.cc wmsession.cc wmwinlist.cc wmtaskbar.cc
 wmwinmenu.cc wmdialog.cc wmabout.cc wmswitch.cc wmstatus.cc
 wmoption.cc wmaction.cc wmcontainer.cc wmclient.cc wmmgr.cc
 wmapp.cc wmframe.cc wmminiicon.cc wmtitle.cc wpixres.cc
 movesize.cc themes.cc decorate.cc browse.cc wmprog.cc atasks.cc
 aworkspaces.cc amailbox.cc aclock.cc acpustatus.cc apppstatus.cc
 aaddressbar.cc objbar.cc aapm.cc atray.cc ysmapp.cc yxtray.cc yurl.cc
 globit.cc amemstatus.cc)


ADD_EXECUTABLE(icewm${EXEEXT} ${ICEWM_SRCS})
set(icewm_pc_flags "${fontconfig_CFLAGS} ${x11_CFLAGS} ${xext_CFLAGS} ${pixbuf_CFLAGS} ${xft_CFLAGS} ${xrandr_CFLAGS} ${xinerama_CFLAGS} ${fribidi_CFLAGS}")
SET_TARGET_PROPERTIES(icewm${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
set(icewm_libs ${sm_LIBS} ${nls_LIBS} ${lsd_LDFLAGS} ${pixbuf_LDFLAGS} ${fontconfig_LDFLAGS} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS} ${fribidi_LDFLAGS} ${EXTRA_LIBS})
TARGET_LINK_LIBRARIES(icewm${EXEEXT} ${icewm_libs})

ADD_EXECUTABLE(genpref${EXEEXT} genpref.cc misc.cc)
TARGET_LINK_LIBRARIES(genpref${EXEEXT} ${EXTRA_LIBS})

IF(CONFIG_FDO_MENUS)
ADD_EXECUTABLE(icewm-menu-fdo${EXEEXT} fdomenu.cc misc.cc)
SET_TARGET_PROPERTIES(icewm-menu-fdo${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${gio_CFLAGS}")
TARGET_LINK_LIBRARIES(icewm-menu-fdo${EXEEXT} ${gio_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
INSTALL(TARGETS icewm-menu-fdo${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icewm-session${EXEEXT} icesm.cc yapp.cc misc.cc mstring.cc upath.cc ytimer.cc yprefs.cc)
SET_TARGET_PROPERTIES(icewm-session${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewm-session${EXEEXT} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmhint${EXEEXT} icewmhint.cc misc.cc)
SET_TARGET_PROPERTIES(icewmhint${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmhint${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icesh${EXEEXT} icesh.cc misc.cc mstring.cc)
SET_TARGET_PROPERTIES(icesh${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icesh${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmbg${EXEEXT} icewmbg.cc ref.cc ${ICE_COMMON_SRCS})
SET_TARGET_PROPERTIES(icewmbg${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmbg${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS}  ${pixbuf_LDFLAGS} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

IF(CONFIG_GUIEVENTS AND (CONFIG_ALSA OR CONFIG_ESD OR CONFIG_OSS OR CONFIG_YIFF))
ADD_EXECUTABLE(icesound${EXEEXT} icesound.cc ycmdline.cc misc.cc mstring.cc)
SET_TARGET_PROPERTIES(icesound${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icesound${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${alsa_LDFLAGS} ${yiff_LDFLAGS} ${esd_LDFLAGS} ${EXTRA_LIBS})
INSTALL(TARGETS icesound${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

IF(NOT LITE)
ADD_EXECUTABLE(icehelp${EXEEXT} icehelp.cc ${ICE_COMMON_SRCS} yscrollbar.cc ref.cc yicon.cc wmconfig.cc ymenu.cc ymenuitem.cc yprefs.cc yscrollview.cc)
SET_TARGET_PROPERTIES(icehelp${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icehelp${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${pixbuf_LDFLAGS} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
INSTALL(TARGETS icehelp${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icewmtray${EXEEXT} icetray.cc ref.cc wmconfig.cc yxtray.cc ${ICE_COMMON_SRCS})
SET_TARGET_PROPERTIES(icewmtray${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmtray${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${pixbuf_LDFLAGS} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

INSTALL(TARGETS icewm${EXEEXT} icesh${EXEEXT} icewm-session${EXEEXT} icewmhint${EXEEXT} icewmbg${EXEEXT} icewmtray${EXEEXT} DESTINATION ${BINDIR})

if(NOT CMAKE_CROSSCOMPILING)
   ADD_CUSTOM_COMMAND(TARGET genpref${EXEEXT}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
      COMMAND genpref${EXEEXT} > lib/preferences
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
   INSTALL(FILES ${CMAKE_BINARY_DIR}/lib/preferences DESTINATION ${LIBDIR})
endif()

#######################################
# print information and some warnings #
#######################################
foreach (_variableName 
		PACKAGE
		VERSION
		DATADIR LIBDIR LOCDIR DOCDIR CFGDIR ICEHELPIDX
		CONFIG_XPM
		CONFIG_IMLIB 
		CONFIG_GDK_PIXBUF_XLIB
		CONFIG_TOOLTIP CONFIG_WINLIST CONFIG_TASKBAR
		CONFIG_ADDRESSBAR CONFIG_TRAY 
		CONFIG_APPLET_MAILBOX 
		CONFIG_APPLET_CPU_STATUS CONFIG_APPLET_NET_STATUS 
		CONFIG_APPLET_CLOCK 
		CONFIG_APPLET_APM 
		CONFIG_WINMENU
		CONFIG_XFREETYPE
		XINERAMA
		)
    message(STATUS "${_variableName} => ${${_variableName}}")
endforeach()

IF(CONFIG_SESSION)
   find_library(HAVE_LIBSM "SM")
   IF(NOT HAVE_LIBSM)
	   message(AUTHOR_WARNING "libSM.so not found, session management probably won't work")
   ENDIF()
   find_library(HAVE_LIBICE "ICE")
   IF(NOT HAVE_LIBICE)
	   message(AUTHOR_WARNING "libICE.so not found, session management probably won't work")
   ENDIF()
ENDIF()

if(NOT CONFIG_XFREETYPE AND NOT CONFIG_COREFONTS)
	message(FATAL_ERROR "Either CONFIG_COREFONTS or CONFIG_COREFONTS must be enabled")
endif()
