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
INCLUDE(CheckCXXCompilerFlag)

SET(CXXFLAGS_COMMON "-pthread -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -I. -DHAVE_CONFIG_H")

set(icewm_img_libs "")
set(icewm_libs "")

if("${CMAKE_BUILD_TYPE}" STREQUAL "" AND ("" STREQUAL "${CMAKE_CXX_FLAGS}"))
    SET(CMAKE_BUILD_TYPE Release)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

# carefully splicing of command line arguments, even from lists
macro(_append varname)
    string(REPLACE ";" " " _apx "${ARGN}")
    if(NOT DEFINED ${varname})
        set(${varname} "${_apx}")
    else()
        set(${varname} "${${varname}} ${_apx}")
    endif()
endmacro()

# I don't need -rdynamic, thanks!
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# apply to test calls
SET(CMAKE_REQUIRED_FLAGS ${CXXFLAGS_COMMON})

CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden CXX_VHI)
if(CXX_VHI)
    _append(CXXFLAGS_COMMON -fvisibility=hidden)
endif()

CHECK_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden CXX_VIHI)
if(CXX_VIHI)
    _append(CXXFLAGS_COMMON -fvisibility-inlines-hidden)
endif()

CHECK_CXX_COMPILER_FLAG(-std=gnu++98 CXX_FORCELEGACY)
if(CXX_FORCELEGACY)
    _append(CXXFLAGS_COMMON -std=gnu++98)
endif()

#CHECK_CXX_COMPILER_FLAG(-Wl,-fuse-ld=gold CXX_LINKER_GOLD)
#if(CXX_LINKER_GOLD)
#    list(APPEND icewm_libs -Wl,-fuse-ld=gold)
#endif()

option(CONFIG_LIBPNG "(Experimental) PNGLIB image loader" on)
option(CONFIG_LIBJPEG "JPEGLIB image loader" on)
option(CONFIG_GDK_PIXBUF_XLIB "GDK-PIXBUF based image loader" off)
option(CONFIG_LIBRSVG "(Optional) SVG image loader" off)
option(CONFIG_XPM "(Optional) XPM image loader" on)
option(CONFIG_I18N "Define to enable internationalization" on)
option(CONFIG_RENDER "Define to enable XRENDER extension" on)
option(CONFIG_XRANDR "Define to enable XRANDR extension" on)
option(CONFIG_SESSION "Define to enable X session management" on)
option(CONFIG_EXTERNAL_TRAY "Define to build tray support in standalone executable" off)
option(ENABLE_NLS "Enable Native Language Support" on)
option(ENABLE_LTO "Enable Link Time Optimization" off)

SET(CONFIG_DEFAULT_THEME "default/default.theme" CACHE STRING "Name of default theme")
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

if(ENABLE_LTO)
    set(CXXFLAGS_COMMON "${CXXFLAGS_COMMON} -flto")
    list(APPEND EXTRA_LIBS -flto)
endif()

# the only used ones...
# for x in `cat funclist` ; do grep $x src/* lib/* && echo $x >> exlist ; done
# perl -e 'print "CHECK_FUNCTION_EXISTS($_ HAVE_".uc($_).")\n" for @ARGV' `cat exlist`
# perl -e 'print "#cmakedefine HAVE_".uc($_)."\n" for @ARGV' `cat exlist`
CHECK_FUNCTION_EXISTS(abort HAVE_ABORT)
CHECK_FUNCTION_EXISTS(backtrace_symbols_fd HAVE_BACKTRACE_SYMBOLS_FD)
CHECK_FUNCTION_EXISTS(basename HAVE_BASENAME)
CHECK_FUNCTION_EXISTS(dup2 HAVE_DUP2)
CHECK_FUNCTION_EXISTS(gethostbyname HAVE_GETHOSTBYNAME)
CHECK_FUNCTION_EXISTS(gethostname HAVE_GETHOSTNAME)
CHECK_FUNCTION_EXISTS(getloadavg HAVE_GETLOADAVG2)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(mblen HAVE_MBLEN)
CHECK_FUNCTION_EXISTS(memchr HAVE_MEMCHR)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(mkdir HAVE_MKDIR)
CHECK_FUNCTION_EXISTS(nl_langinfo HAVE_NL_LANGINFO)
CHECK_FUNCTION_EXISTS(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(socket HAVE_SOCKET)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strcspn HAVE_STRCSPN)
CHECK_FUNCTION_EXISTS(strdup HAVE_STRDUP)
CHECK_FUNCTION_EXISTS(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strncasecmp HAVE_STRNCASECMP)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strsignal HAVE_STRSIGNAL)
CHECK_FUNCTION_EXISTS(strspn HAVE_STRSPN)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(strtoul HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(sysctlbyname HAVE_SYSCTLBYNAME)
CHECK_FUNCTION_EXISTS(sysinfo HAVE_SYSINFO)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)
CHECK_FUNCTION_EXISTS(wordexp HAVE_WORDEXP)
CHECK_FUNCTION_EXISTS(if_indextoname HAVE_IF_INDEXTONAME)
CHECK_FUNCTION_EXISTS(nameindex HAVE_IF_NAMEINDEX)
CHECK_FUNCTION_EXISTS(if_nametoindex HAVE_IF_NAMETOINDEX)

CHECK_LIBRARY_EXISTS (X11 XInternAtoms "" HAVE_XINTERNATOMS)

# perl -e 'do { s/\.|\//_/g; $_=uc $_;  print "#cmakedefine HAVE_$_\n"} for @ARGV' `cat headersDa`
# perl -e 'do {my $orig=$_; s/\.|\//_/g; $_=uc $_; print "CHECK_INCLUDE_FILE_CXX($orig HAVE_$_)\n"} for @ARGV' `cat headersDa`
CHECK_INCLUDE_FILE_CXX(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILE_CXX(kstat.h HAVE_KSTAT_H)
CHECK_INCLUDE_FILE_CXX(langinfo.h HAVE_LANGINFO_H)
CHECK_INCLUDE_FILE_CXX(libintl.h HAVE_LIBINTL_H)
CHECK_INCLUDE_FILE_CXX(limits.h HAVE_LIMITS_H)
CHECK_INCLUDE_FILE_CXX(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILE_CXX(netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILE_CXX(sndfile.h HAVE_SNDFILE_H)
CHECK_INCLUDE_FILE_CXX(stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILE_CXX(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_CXX(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE_CXX(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILE_CXX(sysctl.h HAVE_SYSCTL_H)
CHECK_INCLUDE_FILE_CXX(sys/file.h HAVE_SYS_FILE_H)
CHECK_INCLUDE_FILE_CXX(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/sysctl.h HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE_CXX(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CXX(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CXX(wchar.h HAVE_WCHAR_H)
CHECK_INCLUDE_FILE_CXX(sched.h HAVE_SCHED_H)
CHECK_INCLUDE_FILE_CXX(sys/sched.h HAVE_SYS_SCHED_H)
CHECK_INCLUDE_FILE_CXX(sys/dkstat.h HAVE_SYS_DKSTAT_H)
CHECK_INCLUDE_FILE_CXX(sys/param.h HAVE_SYS_PARAM_H)
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

# XXX: the image loader selection became a big mess but it is hard to revert now
# It would have been a better idea to use different names for the user-specified
# options and do the logics here.
# But for now, this self-adapting code modifies the option variables.

set(HINT_IMG_LOADERS  "Possible combination of image loaders are:
    -DCONFIG_XPM=on -DCONFIG_LIBPNG=on -DCONFIG_GDK_PIXBUF_XLIB=off
    or
    -DCONFIG_XPM=off -DCONFIG_LIBPNG=off -DCONFIG_GDK_PIXBUF_XLIB=on
")
# basic user configuration error check
if(CONFIG_GDK_PIXBUF_XLIB AND NOT CONFIG_XPM)
    #ok
    set(CONFIG_LIBPNG off)
    set(CONFIG_LIBJPEG off)
    if (NOT CONFIG_LIBRSVG)
        message(WARNING "CONFIG_LIBRSVG is recommended for SVG image support")
    endif()
elseif(CONFIG_XPM AND NOT CONFIG_GDK_PIXBUF_XLIB)
    #ok
    if (NOT CONFIG_LIBPNG)
        message(WARNING "LIBPNG is recommended for PNG image support")
    endif()
    if (NOT CONFIG_LIBJPEG)
        message(WARNING "LIBJPEG is advisable for backgrounds in icewmbg")
    endif()
else()
    MESSAGE(FATAL_ERROR "${HINT_IMG_LOADERS}")
endif()

if(CONFIG_GDK_PIXBUF_XLIB)
    pkg_check_modules(pixbuf gdk-pixbuf-xlib-2.0)
    if(NOT pixbuf_FOUND)
        message(FATAL_ERROR "GDX-PIXBUF image loader selected but its dev files were not found.
${HINT_IMG_LOADERS}")
    endif()
    set(CONFIG_XPM off)
    set(CONFIG_LIBPNG off)
    set(CONFIG_IMLIB off)
    set(CONFIG_LIBJPEG off)
    LIST(APPEND icewm_img_libs ${pixbuf_LDFLAGS})
endif()

# this is a) optional, b) loaded through pixbuf anyway
if (CONFIG_LIBRSVG)
    pkg_check_modules(librsvg librsvg-2.0)
    if(NOT librsvg_FOUND)
        message(WARNING "SVG image loader selected but librsvg dev files not found. SVG support will be not available.")
        set(CONFIG_LIBRSVG off)
    endif()
    LIST(APPEND icewm_img_libs ${librsvg_LDFLAGS})
endif()

if(CONFIG_XPM)
    pkg_check_modules(libxpm xpm)
    if(NOT libxpm_FOUND)
        message(FATAL_ERROR "XPM image loader selected but libxpm dev files not found.
${HINT_IMG_LOADERS}")
    endif()
    LIST(APPEND icewm_img_libs ${libxpm_LDFLAGS})
endif()

if(CONFIG_LIBPNG)
    pkg_check_modules(libpng libpng)
    if(NOT libpng_FOUND)
        message(FATAL_ERROR "PNG image loader selected but libpng dev files not found.
${HINT_IMG_LOADERS}")
    endif()
    LIST(APPEND icewm_img_libs ${libpng_LDFLAGS})
endif()

if(CONFIG_LIBJPEG)
    pkg_check_modules(libjpeg libjpeg)
    if(NOT libjpeg_FOUND)
        message(FATAL_ERROR "JPEG image loader selected but libjpeg dev files not found.
${HINT_IMG_LOADERS}")
    endif()
    LIST(APPEND icewm_img_libs ${libjpeg_LDFLAGS})
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

if(CONFIG_XRANDR)
    pkg_check_modules(xrandr xrandr)
    IF(NOT xrandr_FOUND)
        message(WARNING "Xrandr library not found, disabling CONFIG_XRANDR")
        set(CONFIG_XRANDR off)
    ENDIF()
endif()

if(CONFIG_RENDER)
    pkg_check_modules(xrender xrender)
    IF(NOT xrender_FOUND)
        message(WARNING "Xrender library not found, disabling CONFIG_RENDER")
        set(CONFIG_RENDER off)
    ENDIF()
    LIST(APPEND icewm_img_libs ${xrender_LDFLAGS})
endif()

option(CONFIG_COREFONTS "Define to enable X11 core fonts" off)
option(CONFIG_XFREETYPE "Define to enable XFT support" on)
if(CONFIG_XFREETYPE)
    pkg_check_modules(xft xft>=2.0)
    IF(NOT xft_FOUND)
        message(WARNING "xft library (>=2.0) not found, disabling CONFIG_XFREETYPE")
        set(CONFIG_XFREETYPE off)
        set(CONFIG_COREFONTS on)
    ELSE()
        set(CONFIG_XFREETYPE_VALUE 2)
    ENDIF()
else()
    set(CONFIG_COREFONTS on)
endif()

set(ICESOUND "ALSA,AO,OSS,ESound" CACHE STRING "List of supported audio subsystems")
string(TOLOWER ${ICESOUND} ICESOUND)
message(STATUS "Checking audio subsystems from ICESOUND: ${ICESOUND}")

#clear cache
set(ENABLE_ALSA off)
set(ENABLE_AO off)
set(ENABLE_ESD off)
set(ENABLE_OSS off)
set(audio_libs )
set(audio_flags )
# Triggers some cmake bug, use copy-pasta for now
#macro(_addsnd varname flagname)
#        set("${FLAGNAME}" "${${varname}_FOUND}")
#        if("${${varname}_FOUND}" EQUAL 1)
#                list(APPEND audio_libs "${${varname}_LDFLAGS}" "${${varname}_LIBS}")
#                list(APPEND audio_flags "${${varname}_CFLAGS}")
#        endif()
#endmacro()

pkg_check_modules(sndfile sndfile)
if(sndfile_FOUND)
    if(sndfile_FOUND)
        list(APPEND audio_flags "${sndfile_CFLAGS}")
        list(APPEND audio_libs ${sndfile_LDFLAGS})
        list(APPEND audio_libs ${sndfile_LIBS})
    endif()
    if("${ICESOUND}" MATCHES "alsa")
        pkg_check_modules(alsa alsa)
        set(ENABLE_ALSA "${alsa_FOUND}")
        if(alsa_FOUND)
            list(APPEND audio_flags "${alsa_CFLAGS}")
            list(APPEND audio_libs ${alsa_LDFLAGS})
            list(APPEND audio_libs ${alsa_LIBS})
        endif()
    endif()
    if("${ICESOUND}" MATCHES "ao")
        pkg_check_modules(ao ao)
        set(ENABLE_AO "${ao_FOUND}")
        if(ao_FOUND)
            list(APPEND audio_flags "${ao_CFLAGS}")
            list(APPEND audio_libs ${ao_LDFLAGS})
            list(APPEND audio_libs ${ao_LIBS})
        endif()
    endif()
    if("${ICESOUND}" MATCHES "esound|esd")
        pkg_check_modules(esd esound)
        set(ENABLE_ESD "${esd_FOUND}")
        if(esd_FOUND)
            list(APPEND audio_flags "${esd_CFLAGS}")
            list(APPEND audio_libs ${esd_LDFLAGS})
            list(APPEND audio_libs ${esd_LIBS})
        endif()
    endif()
    if("${ICESOUND}" MATCHES "oss")
        CHECK_INCLUDE_FILE_CXX(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
        set(ENABLE_OSS "${HAVE_SYS_SOUNDCARD_H}")
    endif()

    string(REPLACE ";" " " audio_flags "${audio_flags}")
endif(sndfile_FOUND)

option(XINERAMA "Define to enable Xinerama support" on)
if(XINERAMA)
    pkg_check_modules(xinerama xinerama)
    if(NOT xinerama_FOUND)
        message(WARNING "XINERAMA is not supported")
        set(XINERAMA off)
    endif()
endif()

option(CONFIG_FRIBIDI "Define to enable FRIBIDI support" on)
if(CONFIG_FRIBIDI)
    pkg_check_modules(fribidi fribidi)
    if(NOT fribidi_FOUND)
        message(WARNING "FRIBIDI is not supported")
        set(CONFIG_FRIBIDI off)
    endif()
endif()

option(CONFIG_FDO_MENUS "Define to support freedesktop.org style menus" on)
if(CONFIG_FDO_MENUS)
    pkg_check_modules(gio gio-2.0 gio-unix-2.0)
    if(NOT gio_FOUND)
        set(CONFIG_FDO_MENUS off)
        message(WARNING "Freedesktop style menus not supported")
    endif()
endif()

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
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # apparently clang does not support the parameter but the default behavior is what we ant
        SET(CXXFLAGS_COMMON "${CXXFLAGS_COMMON} -Wimplicit-fallthrough")
    else()
        SET(CXXFLAGS_COMMON "${CXXFLAGS_COMMON} -Wimplicit-fallthrough=2")
    endif()
ENDIF()

#######################################
# all checks done, save configuration #
#######################################

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.cmake.h.in" "${CMAKE_BINARY_DIR}/config.h")

#######################################
# declaring targets and build flags   #
#######################################

# pkg-config delivers lists, we need plain compiler options! :-(
foreach(pfx fontconfig xext x11 pixbuf xrandr xrender xft alsa
        esd ao oss xinerama fribidi gio)
    string(REPLACE ";" " " ${pfx}_CFLAGS "${${pfx}_CFLAGS}")
    #message(STATUS "Normalizing argument lists: ${${pfx}_LDFLAGS} ; ${${pfx}_CFLAGS}")
endforeach()

SET(ICE_COMMON_SRCS mstring.cc udir.cc upath.cc yapp.cc yxapp.cc ytimer.cc
    ywindow.cc ypaint.cc ypopup.cc misc.cc ycursor.cc ysocket.cc ypaths.cc
    ylocale.cc yarray.cc ycollections.cc ypipereader.cc yxembed.cc yconfig.cc
    yprefs.cc yfont.cc ypixmap.cc
    yimage_gdk.cc yximage.cc ycolor.cc ytooltip.cc)

if(CONFIG_XFREETYPE)
    list(APPEND ICE_COMMON_SRCS yfontxft.cc)
endif()
if(CONFIG_COREFONTS)
    list(APPEND ICE_COMMON_SRCS yfontcore.cc)
endif()

SET(ITK_SRCS
    ymenu.cc ylabel.cc yscrollview.cc ymenuitem.cc
    yscrollbar.cc ybutton.cc ylistbox.cc yinput.cc
    globit.cc yicon.cc wmconfig.cc wpixres.cc ref.cc
    )

SET(ICEWM_SRCS ${ICE_COMMON_SRCS} ${ITK_SRCS}
    ymsgbox.cc ydialog.cc yurl.cc wmsession.cc
    wmwinlist.cc wmtaskbar.cc wmwinmenu.cc wmdialog.cc
    wmabout.cc wmswitch.cc wmstatus.cc wmoption.cc
    wmcontainer.cc wmclient.cc wmmgr.cc wmapp.cc
    wmframe.cc wmbutton.cc wmminiicon.cc wmtitle.cc
    movesize.cc themes.cc decorate.cc browse.cc
    wmmenu.cc wmprog.cc atasks.cc aworkspaces.cc
    amailbox.cc aclock.cc acpustatus.cc amemstatus.cc
    applet.cc apppstatus.cc aaddressbar.cc objbar.cc
    aapm.cc atray.cc ysmapp.cc yxtray.cc
    )

SET(MISC_SRCS misc.cc yarray.cc mstring.cc ref.cc)

ADD_EXECUTABLE(icewm${EXEEXT} ${ICEWM_SRCS})
set(icewm_pc_flags "${fontconfig_CFLAGS} ${x11_CFLAGS} ${xext_CFLAGS} ${libpng_CFLAGS} ${libxpm_CFLAGS} ${pixbuf_CFLAGS} ${xft_CFLAGS} ${xrandr_CFLAGS} ${xrender_CFLAGS} ${xinerama_CFLAGS} ${fribidi_CFLAGS}")
SET_TARGET_PROPERTIES(icewm${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
list(APPEND icewm_libs ${sm_LIBS} ${nls_LIBS} ${fontconfig_LDFLAGS} ${fribidi_LDFLAGS} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS} ${EXTRA_LIBS})
TARGET_LINK_LIBRARIES(icewm${EXEEXT} ${icewm_libs} ${icewm_img_libs})

ADD_EXECUTABLE(genpref${EXEEXT} genpref.cc ${MISC_SRCS})
TARGET_LINK_LIBRARIES(genpref${EXEEXT} ${EXTRA_LIBS})

IF(CONFIG_FDO_MENUS)
    ADD_EXECUTABLE(icewm-menu-fdo${EXEEXT} fdomenu.cc ${MISC_SRCS})
    SET_TARGET_PROPERTIES(icewm-menu-fdo${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${gio_CFLAGS}")
    TARGET_LINK_LIBRARIES(icewm-menu-fdo${EXEEXT} ${gio_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
    INSTALL(TARGETS icewm-menu-fdo${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icewm-session${EXEEXT} icesm.cc yapp.cc misc.cc mstring.cc upath.cc ytimer.cc yprefs.cc yarray.cc ref.cc)
SET_TARGET_PROPERTIES(icewm-session${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON}")
TARGET_LINK_LIBRARIES(icewm-session${EXEEXT} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmhint${EXEEXT} icewmhint.cc ${MISC_SRCS})
SET_TARGET_PROPERTIES(icewmhint${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmhint${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icesh${EXEEXT} icesh.cc misc.cc mstring.cc ref.cc yarray.cc)
SET_TARGET_PROPERTIES(icesh${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icesh${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmbg${EXEEXT} icewmbg.cc ref.cc ${ICE_COMMON_SRCS})
SET_TARGET_PROPERTIES(icewmbg${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmbg${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${fribidi_LDFLAGS} ${xrandr_LDFLAGS}  ${icewm_img_libs} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

IF(ENABLE_ALSA OR ENABLE_AO OR ENABLE_ESD OR ENABLE_OSS)
    ADD_EXECUTABLE(icesound${EXEEXT} icesound.cc upath.cc misc.cc mstring.cc ytimer.cc yapp.cc yprefs.cc yarray.cc ref.cc)
    SET_TARGET_PROPERTIES(icesound${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags} ${audio_flags}")
    TARGET_LINK_LIBRARIES(icesound${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${audio_libs} ${EXTRA_LIBS})
    INSTALL(TARGETS icesound${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icehelp${EXEEXT} icehelp.cc ${ICE_COMMON_SRCS} yscrollbar.cc ref.cc yicon.cc wmconfig.cc ymenu.cc ymenuitem.cc yprefs.cc yscrollview.cc)
SET_TARGET_PROPERTIES(icehelp${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icehelp${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${fribidi_LDFLAGS} ${xrandr_LDFLAGS} ${icewm_img_libs} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
INSTALL(TARGETS icehelp${EXEEXT} DESTINATION ${BINDIR})

IF(CONFIG_EXTERNAL_TRAY)
ADD_EXECUTABLE(icewmtray${EXEEXT} icetray.cc ref.cc wmconfig.cc yxtray.cc wmoption.cc ${ICE_COMMON_SRCS})
SET_TARGET_PROPERTIES(icewmtray${EXEEXT} PROPERTIES COMPILE_FLAGS "${CXXFLAGS_COMMON} ${icewm_pc_flags}")
TARGET_LINK_LIBRARIES(icewmtray${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS} ${nls_LIBS} ${icewm_img_libs} ${EXTRA_LIBS})
ENDIF()

INSTALL(TARGETS icewm${EXEEXT} icesh${EXEEXT} icewm-session${EXEEXT} icewmhint${EXEEXT} icewmbg${EXEEXT} DESTINATION ${BINDIR})

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
        CONFIG_IMLIB
        CONFIG_GDK_PIXBUF_XLIB
        CONFIG_LIBRSVG
        CONFIG_XPM
        CONFIG_LIBJPEG
        CONFIG_LIBPNG
        CONFIG_RENDER
        CONFIG_XFREETYPE
        CONFIG_COREFONTS
        CONFIG_FRIBIDI
        CONFIG_EXTERNAL_TRAY
        XINERAMA
        ENABLE_LTO
        ENABLE_ALSA
        ENABLE_AO
        ENABLE_ESD
        ENABLE_OSS
        ENABLE_NLS
        )
    if("${${_variableName}}" STREQUAL "")
        message(STATUS "${_variableName} => OFF")
    else()
        message(STATUS "${_variableName} => ${${_variableName}}")
    endif()
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

# vim: set sw=4 ts=4 et:
