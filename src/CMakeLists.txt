cmake_minimum_required(VERSION 3.2)

PROJECT(ICEWM CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
# TODO: drop this warning around xmas 2020 and raise cmake_minimum_required instead; Debian Jessie EOLs around 2020-06
if(${CMAKE_VERSION} VERSION_LESS "3.1")
    message(WARNING "!! Ancient cmake version detected, please pass flags for C++11 enabling manually through CXXFLAGS environment variable or override CMAKE_CXX_FLAGS")
endif()

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckTypeSize)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(FindPkgConfig)
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckCXXCompilerFlag)

SET(CXXFLAGS_COMMON -pthread -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -DHAVE_CONFIG_H -include "${CMAKE_BINARY_DIR}/config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(icewm_libs "")
# optimize for release if not defined explicitly, can be overriden with CMAKE_BUILD_TYPE "None"
if("${CMAKE_BUILD_TYPE}" STREQUAL "" AND ("" STREQUAL "${CMAKE_CXX_FLAGS}"))
    SET(CMAKE_BUILD_TYPE Release)
endif()

# I don't need -rdynamic, thanks!
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

SET(CMAKE_REQUIRED_FLAGS )

CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden CXX_VHI)
if(CXX_VHI)
    list(APPEND CXXFLAGS_COMMON -fvisibility=hidden)
endif()

CHECK_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden CXX_VIHI)
if(CXX_VIHI)
    list(APPEND CXXFLAGS_COMMON -fvisibility-inlines-hidden)
endif()

# teach GCC7+ to interpret our comments
CHECK_CXX_COMPILER_FLAG(-Wimplicit-fallthrough=2 HAS_FALLTHROUGH_OPTION)
if(HAS_FALLTHROUGH_OPTION)
    list(APPEND CXXFLAGS_COMMON -Wimplicit-fallthrough=2)
else()
    CHECK_CXX_COMPILER_FLAG(-Wimplicit-fallthrough HAS_FALLTHROUGH_OPTION)
    if(HAS_FALLTHROUGH_OPTION)
        list(APPEND CXXFLAGS_COMMON -Wimplicit-fallthrough)
    endif()
endif()

#CHECK_CXX_COMPILER_FLAG(-Wl,-fuse-ld=gold CXX_LINKER_GOLD)
#if(CXX_LINKER_GOLD)
#    list(APPEND icewm_libs -Wl,-fuse-ld=gold)
#endif()

option(CONFIG_LIBPNG "PNGLIB image loader" off)
option(CONFIG_LIBJPEG "JPEGLIB image loader" off)
option(CONFIG_GDK_PIXBUF_XLIB "GDK-PIXBUF based image loader" on)
option(CONFIG_LIBRSVG "Optional SVG images for GDK-PIXBUF" off)
option(CONFIG_XPM "XPM image loader" on)
option(CONFIG_I18N "Define to enable internationalization" on)
option(CONFIG_XRANDR "Define to enable XRANDR extension" on)
option(CONFIG_SESSION "Define to enable X session management" on)
option(CONFIG_EXTERNAL_TRAY "Define for external systray (deprecated)" off)
option(ENABLE_NLS "Enable Native Language Support" on)
option(ENABLE_LTO "Enable Link Time Optimization" off)
option(ENABLE_LOGEVENTS "Enable logging of X11 events" off)

set(LOGEVENTS ${ENABLE_LOGEVENTS})
SET(CONFIG_DEFAULT_THEME "default/default.theme" CACHE STRING "Name of default theme")
SET(CONFIG_UNICODE_SET "" CACHE STRING "Your iconv unicode set in machine endian encoding (e.g. WCHAR_T, UCS-4-INTERNAL, UCS-4LE, UCS-4BE)")
# turn off when empty, see config.h template for details
IF("" STREQUAL "${CONFIG_UNICODE_SET}")
    SET(CONFIG_UNICODE_SET OFF)
ELSE()
    MESSAGE(STATUS "Configured custom unicode set: ${CONFIG_UNICODE_SET}.")
ENDIF()
SET(XTERMCMD xterm CACHE STRING "Default terminal emulator for X11")

# TODO: find some common helper to handle that variables
if(ENABLE_NLS OR CONFIG_I18N)
    find_library(CONFIG_LIBICONV iconv)
    if(CONFIG_LIBICONV)
        LIST(APPEND nls_LIBS iconv)
    endif()
    INCLUDE(FindIntl)
    if(Intl_FOUND)
        LIST(APPEND nls_LIBS ${Intl_LIBRARIES})
        LIST(APPEND nls_CFLAGS -I${Intl_INCLUDE_DIRS})
    endif()
endif()

if(ENABLE_LTO AND ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND CXXFLAGS_COMMON -flto)
    list(APPEND EXTRA_LIBS -flto)
endif()

# the only used ones...
# for x in `cat funclist` ; do grep $x src/* lib/* && echo $x >> exlist ; done
# perl -e 'print "CHECK_FUNCTION_EXISTS($_ HAVE_".uc($_).")\n" for @ARGV' `cat exlist`
# perl -e 'print "#cmakedefine HAVE_".uc($_)."\n" for @ARGV' `cat exlist`
CHECK_FUNCTION_EXISTS(backtrace_symbols_fd HAVE_BACKTRACE_SYMBOLS_FD)
CHECK_FUNCTION_EXISTS(memrchr HAVE_MEMRCHR)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(sysctl HAVE_SYSCTL)
CHECK_FUNCTION_EXISTS(sysctlbyname HAVE_SYSCTLBYNAME)

# perl -e 'do { s/\.|\//_/g; $_=uc $_;  print "#cmakedefine HAVE_$_\n"} for @ARGV' `cat headersDa`
# perl -e 'do {my $orig=$_; s/\.|\//_/g; $_=uc $_; print "CHECK_INCLUDE_FILE_CXX($orig HAVE_$_)\n"} for @ARGV' `cat headersDa`
CHECK_INCLUDE_FILE_CXX(execinfo.h HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE_CXX(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILE_CXX(libgen.h HAVE_LIBGEN_H)
CHECK_INCLUDE_FILE_CXX(sched.h HAVE_SCHED_H)
CHECK_INCLUDE_FILE_CXX(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILE_CXX(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE_CXX(sys/param.h HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE_CXX(sys/sched.h HAVE_SYS_SCHED_H "-include /usr/include/sched.h")
CHECK_INCLUDE_FILE_CXX(sys/sysctl.h HAVE_SYS_SYSCTL_H "-include /usr/include/sys/types.h")
CHECK_INCLUDE_FILE_CXX(uvm/uvm_param.h HAVE_UVM_UVM_PARAM_H)

CHECK_TYPE_SIZE(char SIZEOF_CHAR)
CHECK_TYPE_SIZE(long SIZEOF_LONG)
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(short SIZEOF_SHORT)

#########################################################
# fiting flags to options and available system features #
#########################################################

pkg_check_modules(fontconfig fontconfig)
pkg_check_modules(xrender REQUIRED xrender)
pkg_check_modules(xcomposite REQUIRED xcomposite)
pkg_check_modules(xdamage REQUIRED xdamage)
pkg_check_modules(xfixes REQUIRED xfixes)
pkg_check_modules(xext REQUIRED xext)
pkg_check_modules(x11 REQUIRED x11)

CHECK_LIBRARY_EXISTS(X11 XInternAtoms ${x11_LIBDIR} HAVE_XINTERNATOMS)

set(icewm_img_libs ${xrender_LDFLAGS} ${xcomposite_LDFLAGS} ${xdamage_LDFLAGS} ${xfixes_LDFLAGS})

# It would be a good idea to use different names for the image options and do the
# logics here.  For now, this self-adapting code modifies the option variables.

set(HINT_CURSORS
[=[
 Cursor support requires either -DCONFIG_XPM=on or -DCONFIG_IMLIB=on.
]=])

set(HINT_IMG_LOADERS
[=[
 There is a choice to be made between two image loaders.
 Either 'yimage_gdk' must be enabled by -DCONFIG_GDK_PIXBUF_XLIB=on,
 or else 'yximage' must be enabled by -DCONFIG_LIBPNG=on.
 The latter requires -DCONFIG_XPM=on, while -DCONFIG_LIBJPEG=on is recommended.
 The first provides optional support for SVG with -DCONFIG_LIBRSVG=on.
 ]=] ${HINT_CURSORS})

# image configuration error check
if(CONFIG_GDK_PIXBUF_XLIB)
    #ok
    set(CONFIG_LIBPNG off)
    set(CONFIG_LIBJPEG off)
    if (NOT CONFIG_LIBRSVG)
        message(WARNING "CONFIG_LIBRSVG is recommended for SVG image support")
    endif()
elseif(CONFIG_XPM AND CONFIG_LIBPNG)
    #ok
    if (NOT CONFIG_LIBJPEG)
        message(WARNING "LIBJPEG is advisable for backgrounds in icewmbg")
    endif()
    set(CONFIG_LIBRSVG off)
else()
    MESSAGE(FATAL_ERROR ${HINT_IMG_LOADERS})
endif()

# cursor configuration error check
if(CONFIG_XPM)
    #ok
    set(CONFIG_IMLIB off)
elseif(CONFIG_IMLIB)
    #ok
    set(CONFIG_XPM off)
else()
    MESSAGE(FATAL_ERROR ${HINT_CURSORS})
endif()

if(CONFIG_GDK_PIXBUF_XLIB)
    pkg_check_modules(pixbuf gdk-pixbuf-xlib-2.0)
    if(NOT pixbuf_FOUND)
        message(FATAL_ERROR "GDX-PIXBUF image loader selected but its dev files were not found."
${HINT_IMG_LOADERS})
    endif()
    set(CONFIG_LIBPNG off)
    set(CONFIG_LIBJPEG off)
    LIST(APPEND icewm_img_libs ${pixbuf_LDFLAGS})
endif()

# this is a) optional, b) loaded through pixbuf anyway
if (CONFIG_LIBRSVG)
    pkg_check_modules(librsvg librsvg-2.0)
    if(NOT librsvg_FOUND)
        message(WARNING
                "SVG image loader selected but librsvg-2.0 dev files not found."
                "SVG support will be not available.")
        set(CONFIG_LIBRSVG off)
    else()
        LIST(APPEND icewm_img_libs ${librsvg_LDFLAGS})
    endif()
endif()

if(CONFIG_XPM)
    pkg_check_modules(libxpm xpm)
    if(NOT libxpm_FOUND)
        message(FATAL_ERROR
                "XPM image loader selected but libXpm dev files not found."
                ${HINT_IMG_LOADERS})
    endif()
    LIST(APPEND icewm_img_libs ${libxpm_LDFLAGS})
endif()

if(CONFIG_LIBPNG)
    pkg_check_modules(libpng libpng)
    if(NOT libpng_FOUND)
        message(FATAL_ERROR
                "PNG image loader selected but libpng dev files not found."
                ${HINT_IMG_LOADERS})
    endif()
    LIST(APPEND icewm_img_libs ${libpng_LDFLAGS})
endif()

if(CONFIG_LIBJPEG)
    pkg_check_modules(libjpeg libjpeg)
    if(NOT libjpeg_FOUND)
        message(FATAL_ERROR
                "JPEG image loader selected but libjpeg dev files not found."
                "Please install libjpeg-dev (or similar)."
                "Otherwise use -DCONFIG_LIBJPEG=off.")
    endif()
    LIST(APPEND icewm_img_libs ${libjpeg_LDFLAGS})
endif()

if(CONFIG_IMLIB)
    pkg_check_modules(libimlib imlib)
    if(NOT libimlib_FOUND)
        message(FATAL_ERROR "IMLIB selected but libImlib dev files not found."
        ${HINT_CURSORS})
    endif()
    LIST(APPEND icewm_img_libs ${libimlib_LDFLAGS})
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
    if("${ICESOUND}" MATCHES "oss")
        CHECK_INCLUDE_FILE_CXX(sys/soundcard.h HAVE_SYS_SOUNDCARD_H)
        set(ENABLE_OSS "${HAVE_SYS_SOUNDCARD_H}")
    endif()
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
        message(WARNING "Freedesktop style menus not supported (libglib2.0-dev or similar package required)")
    endif()
endif()

option(DEBUG "Define if you want to debug IceWM" off)

#######################################
# some extra sugar where available    #
#######################################

IF(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    SET(LDFLAGS_ORIG ${CMAKE_EXE_LINKER_FLAGS})
    SET(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS_ORIG} -Wl,--as-needed ")
    CHECK_CXX_SOURCE_COMPILES("int main() {return 0;}" USE_WLASNEEDED)
    IF(NOT USE_WLASNEEDED)
        SET(CMAKE_EXE_LINKER_FLAGS ${LDFLAGS_ORIG})
    ENDIF()
ENDIF()
IF(${CMAKE_HOST_SYSTEM_NAME} MATCHES "SunOS")
    list(APPEND EXTRA_LIBS -lsocket)
ENDIF()

#######################################
# all checks done, save configuration #
#######################################

CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.cmake.h.in" "${CMAKE_BINARY_DIR}/config.h")

#######################################
# declaring targets and build flags   #
#######################################

SET(MISC_SRCS logevent.cc misc.cc yfileio.cc ytime.cc yarray.cc mstring.cc ref.cc)

SET(ICE_COMMON_SRCS udir.cc upath.cc yapp.cc yxapp.cc ytimer.cc yprefs.cc
    ywindow.cc ypaint.cc ypopup.cc ycursor.cc ysocket.cc ypaths.cc
    ylocale.cc ypipereader.cc yxembed.cc yconfig.cc yfont.cc ypixmap.cc
    yimage_gdk.cc yximage.cc ycolor.cc ytooltip.cc ${MISC_SRCS})

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
    akeyboard.cc aapm.cc atray.cc ysmapp.cc yxtray.cc
    )

ADD_EXECUTABLE(icewm${EXEEXT} ${ICEWM_SRCS})
set(icewm_pc_flags ${fontconfig_CFLAGS} ${x11_CFLAGS} ${xext_CFLAGS} ${libpng_CFLAGS} ${libxpm_CFLAGS} ${pixbuf_CFLAGS} ${xft_CFLAGS} ${xrandr_CFLAGS} ${xrender_CFLAGS} ${xinerama_CFLAGS} ${fribidi_CFLAGS} ${nls_CFLAGS})
target_compile_options(icewm${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
set(icewm_libs ${sm_LIBS} ${nls_LIBS} ${fontconfig_LDFLAGS} ${fribidi_LDFLAGS} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS})
TARGET_LINK_LIBRARIES(icewm${EXEEXT} ${icewm_libs} ${icewm_img_libs} ${EXTRA_LIBS})

ADD_EXECUTABLE(genpref${EXEEXT} genpref.cc ${MISC_SRCS})
set(genpref_pc_flags ${x11_CFLAGS}  ${nls_CFLAGS})
target_compile_options(genpref${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${genpref_pc_flags})
TARGET_LINK_LIBRARIES(genpref${EXEEXT} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(strtest EXCLUDE_FROM_ALL strtest.cc upath.cc udir.cc ${MISC_SRCS})
target_compile_options(strtest PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(strtest ${nls_LIBS})

add_test(strtest ${CMAKE_BINARY_DIR}/strtest)

IF(CONFIG_FDO_MENUS)
    ADD_EXECUTABLE(icewm-menu-fdo${EXEEXT} ycollections.cc fdomenu.cc ${MISC_SRCS})
    target_compile_options(icewm-menu-fdo${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${x11_CFLAGS} ${gio_CFLAGS}  ${nls_CFLAGS})
    TARGET_LINK_LIBRARIES(icewm-menu-fdo${EXEEXT} ${x11_LDFLAGS} ${gio_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
    INSTALL(TARGETS icewm-menu-fdo${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icewm-session${EXEEXT} icesm.cc yapp.cc upath.cc ytimer.cc yprefs.cc ${MISC_SRCS})
set(icewm_session_pc_flags ${x11_CFLAGS}  ${nls_CFLAGS})
target_compile_options(icewm-session${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_session_pc_flags})
TARGET_LINK_LIBRARIES(icewm-session${EXEEXT} ${nls_LIBS} ${x11_LDFLAGS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmhint${EXEEXT} icewmhint.cc ${MISC_SRCS})
target_compile_options(icewmhint${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(icewmhint${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icesh${EXEEXT} icesh.cc ${MISC_SRCS})
target_compile_options(icesh${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(icesh${EXEEXT} ${xext_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

ADD_EXECUTABLE(icewmbg${EXEEXT} icewmbg.cc ref.cc ${ICE_COMMON_SRCS})
target_compile_options(icewmbg${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(icewmbg${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${fribidi_LDFLAGS} ${xrandr_LDFLAGS}  ${icewm_img_libs} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})

IF(ENABLE_ALSA OR ENABLE_AO OR ENABLE_OSS)
    ADD_EXECUTABLE(icesound${EXEEXT} icesound.cc upath.cc ytimer.cc yapp.cc yprefs.cc ${MISC_SRCS})
    target_compile_options(icesound${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags} ${audio_flags})
    TARGET_LINK_LIBRARIES(icesound${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${audio_libs} ${EXTRA_LIBS})
    INSTALL(TARGETS icesound${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

ADD_EXECUTABLE(icehelp${EXEEXT} icehelp.cc ${ICE_COMMON_SRCS} yscrollbar.cc ref.cc yicon.cc wmconfig.cc ymenu.cc ymenuitem.cc yprefs.cc yscrollview.cc)
target_compile_options(icehelp${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(icehelp${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${fribidi_LDFLAGS} ${xrandr_LDFLAGS} ${icewm_img_libs} ${xinerama_LDFLAGS} ${nls_LIBS} ${EXTRA_LIBS})
INSTALL(TARGETS icehelp${EXEEXT} DESTINATION ${BINDIR})

IF(CONFIG_EXTERNAL_TRAY)
ADD_EXECUTABLE(icewmtray${EXEEXT} icetray.cc ref.cc wmconfig.cc yxtray.cc wmoption.cc ${ICE_COMMON_SRCS})
target_compile_options(icewmtray${EXEEXT} PUBLIC ${CXXFLAGS_COMMON} ${icewm_pc_flags})
TARGET_LINK_LIBRARIES(icewmtray${EXEEXT} ${xext_LDFLAGS} ${x11_LDFLAGS} ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS} ${nls_LIBS} ${icewm_img_libs} ${EXTRA_LIBS})
ENDIF()

INSTALL(TARGETS icewm${EXEEXT} icesh${EXEEXT} icewm-session${EXEEXT} icewmhint${EXEEXT} icewmbg${EXEEXT} DESTINATION ${BINDIR})

if(NOT CMAKE_CROSSCOMPILING)
    ADD_CUSTOM_COMMAND(TARGET genpref${EXEEXT}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
        COMMAND genpref${EXEEXT} -o ${CMAKE_BINARY_DIR}/lib/preferences
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
        CONFIG_GDK_PIXBUF_XLIB
        CONFIG_LIBRSVG
        CONFIG_XPM
        CONFIG_LIBPNG
        CONFIG_LIBJPEG
        CONFIG_IMLIB
        CONFIG_XFREETYPE
        CONFIG_COREFONTS
        CONFIG_FRIBIDI
        CONFIG_EXTERNAL_TRAY
        XINERAMA
        ENABLE_LTO
        ENABLE_ALSA
        ENABLE_AO
        ENABLE_OSS
        ENABLE_NLS
        ENABLE_LOGEVENTS
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
