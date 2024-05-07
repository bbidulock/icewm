
# to use target_link_options
cmake_minimum_required(VERSION 3.13)

PROJECT(ICEWM CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

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

unset(icewm_libs)
unset(Intl_INCLUDE_DIRS)

macro(concat_dedup _result_var)
    set(${_result_var} ${ARGN})
    list(REMOVE_DUPLICATES ${_result_var})
endmacro()

option(AUTO_INC_HEADERS "Guess related headers and include in the build config (developer comfort feature)" off)
macro(do_auto_inc_headers _in_out_var)
    if(AUTO_INC_HEADERS)
        set(auto_inc_headers_tmp)
        foreach(src ${${_in_out_var}})
            list(APPEND auto_inc_headers_tmp ${src})
            cmake_path(REPLACE_EXTENSION src .h OUTPUT_VARIABLE h_cand)
            message("Looking for ${h_cand}")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${h_cand}")
                list(APPEND auto_inc_headers_tmp "${h_cand}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES auto_inc_headers_tmp)
        set(${_in_out_var} ${auto_inc_headers_tmp})
    endif()
endmacro()

macro(pkg_check_or_fail _prefix _what)
    clear_pkg_config(${_prefix})
    pkg_check_modules(${_prefix} ${_what})
    if(NOT ${_prefix}_FOUND)
        message(FATAL_ERROR ${ARGN})
    endif()
endmacro()

# drop remainings variables of pkg_config's work from the current scope
macro(clear_pkg_config _prefix)
    foreach(_i "_LIBRARIES" "_LINK_LIBRARIES" "_LIBRARY_DIRS" "_LDFLAGS" "_LDFLAGS_OTHER" "_INCLUDE_DIRS" "_CFLAGS" "_CFLAGS_OTHER")
        unset("${_prefix}${_i}")
    endforeach()
endmacro()

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

option(CONFIG_IMLIB2 "Imlib2 image loader" on)
option(CONFIG_LIBPNG "PNGLIB image loader" off)
option(CONFIG_LIBJPEG "JPEGLIB image loader" off)
option(CONFIG_GDK_PIXBUF_XLIB "GDK-PIXBUF image loader" off)
option(CONFIG_LIBRSVG "Support SVG icons using librsvg" off)
option(CONFIG_NANOSVG "Support SVG icons using nanosvg" off)
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
SET(CONFIG_DEFAULT_BACKGROUND "" CACHE STRING "Default background image or directory")
SET(CONFIG_UNICODE_SET "" CACHE STRING "Your iconv unicode set in machine endian encoding (e.g. WCHAR_T, UCS-4-INTERNAL, UCS-4LE, UCS-4BE)")
# turn off when empty, see config.h template for details
IF("" STREQUAL "${CONFIG_UNICODE_SET}")
    SET(CONFIG_UNICODE_SET OFF)
ELSE()
    MESSAGE(STATUS "Configured custom unicode set: ${CONFIG_UNICODE_SET}.")
ENDIF()
SET(XTERMCMD xterm CACHE STRING "Default terminal emulator for X11")

if(ENABLE_NLS OR CONFIG_I18N)
    find_library(CONFIG_LIBICONV iconv)
    if(CONFIG_LIBICONV)
        LIST(APPEND nls_LIBS iconv)
    endif()
# XXX: once cmake 3.20 became standard, replace this block with dependency on
# Intl::Intl interface target.
    include(FindIntl)
    if(Intl_FOUND)
        list(APPEND nls_LIBS ${Intl_LIBRARIES})
    endif()
endif()

if(ENABLE_LTO AND ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    # https://stackoverflow.com/questions/72218980/gcc-v12-1-warning-about-serial-compilation
    check_cxx_compiler_flag(-flto=auto USE_LTOAUTO)
    if(USE_LTOAUTO)
        list(APPEND CXXFLAGS_COMMON -flto=auto)
    else()
        list(APPEND CXXFLAGS_COMMON -flto)
    endif()
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
CHECK_INCLUDE_FILE_CXX(sched.h HAVE_SCHED_H)
CHECK_INCLUDE_FILE_CXX(sys/sched.h HAVE_SYS_SCHED_H "-include /usr/include/sched.h")
CHECK_INCLUDE_FILE_CXX(sys/sysctl.h HAVE_SYS_SYSCTL_H "-include /usr/include/sys/types.h")
CHECK_INCLUDE_FILE_CXX(uvm/uvm_param.h HAVE_UVM_UVM_PARAM_H)

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
 Cursor support requires either -DCONFIG_XPM=on or -DCONFIG_IMLIB2=on.
]=])

set(HINT_IMG_LOADERS
[=[
 There is a choice to be made between two image loaders.
 Either 'yimage_gdk' must be enabled by -DCONFIG_GDK_PIXBUF_XLIB=on,
 or 'imlib2' must be enabled by -DCONFIG_IMLIB2=on,
 or else 'yximage' must be enabled by -DCONFIG_LIBPNG=on.
 The latter requires -DCONFIG_XPM=on, while -DCONFIG_LIBJPEG=on is recommended.
 Support SVG icons with either -DCONFIG_LIBRSVG=on or -DCONFIG_NANOSVG=on.
 ]=] ${HINT_CURSORS})

# image configuration error check
if(CONFIG_IMLIB2 OR CONFIG_GDK_PIXBUF_XLIB)
    if (NOT CONFIG_LIBRSVG AND NOT CONFIG_NANOSVG)
        message(WARNING "Support for SVG icons was not enabled")
    endif()
elseif(CONFIG_XPM AND CONFIG_LIBPNG)
    if (NOT CONFIG_LIBJPEG)
        message(WARNING "LIBJPEG is advisable for backgrounds in icewmbg")
    endif()
    set(CONFIG_LIBRSVG off)
    set(CONFIG_NANOSVG off)
else()
    MESSAGE(FATAL_ERROR ${HINT_IMG_LOADERS})
endif()

if(CONFIG_IMLIB2)
    pkg_check_or_fail(libimlib2 imlib2 "IMLIB2 selected but Imlib2 dev files not found.")
    set(CONFIG_GDK_PIXBUF_XLIB off)
    set(CONFIG_LIBPNG off)
    set(CONFIG_LIBJPEG off)
    LIST(APPEND icewm_img_libs ${libimlib2_LDFLAGS})
endif()

if(CONFIG_GDK_PIXBUF_XLIB)
    pkg_check_or_fail(pixbuf gdk-pixbuf-xlib-2.0
        "GDX-PIXBUF image loader selected but its dev files were not found."
        ${HINT_IMG_LOADERS})
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
                "SVG support will not be available.")
        set(CONFIG_LIBRSVG off)
    else()
        LIST(APPEND icewm_img_libs ${librsvg_LDFLAGS})
        set(CONFIG_NANOSVG off)
    endif()
endif()

if (CONFIG_NANOSVG)
    set(hub https://raw.githubusercontent.com/memononen/nanosvg/master/src)
    foreach(nano nanosvg.h nanosvgrast.h)
        message(STATUS "Checking for src/${nano}")
        set(src ${CMAKE_SOURCE_DIR}/src/${nano})
        if (NOT EXISTS ${src})
            set(url ${hub}/${nano})
            file(DOWNLOAD ${url} ${src} INACTIVITY_TIMEOUT 3 STATUS down)
            list(GET down 0 stat)
            list(GET down 1 mesg)
            if(NOT ${stat} EQUAL 0 OR NOT EXISTS ${src})
                message(WARNING "Cannot download ${url} to ${src}: ${mesg}")
                set(CONFIG_NANOSVG off)
                break()
            endif()
        endif()
    endforeach()
endif()

# cursor configuration error check
if(CONFIG_XPM)
    #ok
elseif(CONFIG_IMLIB2)
    #ok
else()
    MESSAGE(FATAL_ERROR ${HINT_CURSORS})
endif()

if(CONFIG_XPM)
    pkg_check_or_fail(libxpm xpm "XPM image loader selected but libXpm dev files not found."
        ${HINT_IMG_LOADERS})
    LIST(APPEND icewm_img_libs ${libxpm_LDFLAGS})
endif()

if(CONFIG_LIBPNG)
    pkg_check_or_fail(libpng libpng "PNG image loader selected but libpng dev files not found."
        ${HINT_IMG_LOADERS})
    LIST(APPEND icewm_img_libs ${libpng_LDFLAGS})
endif()

if(CONFIG_LIBJPEG)
    pkg_check_or_fail(libjpeg libjpeg "JPEG image loader selected but libjpeg dev files not found."
        "Please install libjpeg-dev (or similar)."
        "Otherwise use -DCONFIG_LIBJPEG=off.")
    LIST(APPEND icewm_img_libs ${libjpeg_LDFLAGS})
endif()

IF(CONFIG_SESSION)
    LIST(APPEND sm_LIBS SM ICE)
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
message(STATUS "Checking audio subsystems for ICESOUND: ${ICESOUND}")

# clear cache, just in case that user has set them as command line options
set(ENABLE_ALSA off)
set(ENABLE_AO off)
set(ENABLE_OSS off)

pkg_check_modules(sndfile sndfile)
if(sndfile_FOUND)
    if(sndfile_FOUND)
        list(APPEND audio_flags ${sndfile_CFLAGS})
        list(APPEND audio_libs ${sndfile_LDFLAGS} ${sndfile_LIBS})
    endif()
    if("${ICESOUND}" MATCHES "alsa")
        pkg_check_modules(alsa alsa)
        set(ENABLE_ALSA ${alsa_FOUND})
        if(alsa_FOUND)
            list(APPEND audio_flags ${alsa_CFLAGS})
            list(APPEND audio_libs ${alsa_LDFLAGS} ${alsa_LIBS})
        endif()
    endif()
    if("${ICESOUND}" MATCHES "ao")
        pkg_check_modules(ao ao)
        set(ENABLE_AO "${ao_FOUND}")
        if(ao_FOUND)
            list(APPEND audio_flags ${ao_CFLAGS})
            list(APPEND audio_libs ${ao_LDFLAGS} ${ao_LIBS})
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
    set(hint "Freedesktop style menus enabled (-DCONFIG_FDO_MENUS=on) but libglib2.0-dev or similar package not found")
    pkg_check_or_fail(gio gio-2.0 gio-unix-2.0 ${hint})
    pkg_check_or_fail(gio_unix gio-unix-2.0 ${hint})
endif()

option(DEBUG "Define if you want to debug IceWM" off)
option(PRECON "Define to enable assertions in IceWM" off)

#######################################
# some extra sugar where available    #
#######################################

IF(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    check_cxx_compiler_flag(-Wl,--as-needed USE_WLASNEEDED)
    if(USE_WLASNEEDED)
        list(APPEND EXTRA_LINKER_FLAGS -Wl,--as-needed)
    endif()
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

set(icewm_pc_flags ${CXXFLAGS_COMMON} ${fontconfig_CFLAGS} ${xft_CFLAGS}
                   ${librsvg_CFLAGS} ${pixbuf_CFLAGS} ${libimlib2_CFLAGS}
                   ${libpng_CFLAGS} ${libxpm_CFLAGS} ${xrender_CFLAGS}
                   ${xrandr_CFLAGS} ${xinerama_CFLAGS} ${xext_CFLAGS}
                   ${x11_CFLAGS} ${fribidi_CFLAGS} ${nls_CFLAGS})

SET(ICE_COMMON_SRCS udir.cc upath.cc yapp.cc yxapp.cc ytimer.cc yprefs.cc
                    ywindow.cc ypaint.cc ypopup.cc ycursor.cc ysocket.cc
                    ypipereader.cc yxembed.cc yconfig.cc yfont.cc ysvg.cc
                    ypixmap.cc yimage2.cc yimage_gdk.cc yximage.cc ycolor.cc
                    ytooltip.cc ylocale.cc yarray.cc yfileio.cc ytime.cc
                    ystring.cc mstring.cc ref.cc keysyms.cc logevent.cc misc.cc)

if(CONFIG_XFREETYPE)
    list(APPEND ICE_COMMON_SRCS yfontxft.cc)
endif()
if(CONFIG_COREFONTS)
    list(APPEND ICE_COMMON_SRCS yfontcore.cc)
endif()

list(REMOVE_DUPLICATES icewm_pc_flags)

do_auto_inc_headers(ICE_COMMON_SRCS)
add_library(ice STATIC ${ICE_COMMON_SRCS})
target_compile_options(ice PUBLIC ${icewm_pc_flags})
target_include_directories(ice PUBLIC ${Intl_INCLUDE_DIRS})

SET(ITK_SRCS ymenu.cc ylabel.cc yscrollview.cc ymenuitem.cc
             yscrollbar.cc ybutton.cc ylistbox.cc yinputline.cc
             globit.cc yicon.cc wmconfig.cc wmsave.cc wpixres.cc)

do_auto_inc_headers(ITK_SRCS)
add_library(itk STATIC ${ITK_SRCS})
target_compile_options(itk PUBLIC ${icewm_pc_flags})

SET(ICEWM_SRCS
    ymsgbox.cc ydialog.cc yurl.cc wmsession.cc
    wmwinlist.cc wmtaskbar.cc wmwinmenu.cc wmdialog.cc
    wmabout.cc wmswitch.cc wmstatus.cc wmoption.cc
    wmcontainer.cc wmclient.cc wmmgr.cc wmapp.cc
    wmframe.cc wmbutton.cc wmminiicon.cc wmtitle.cc
    movesize.cc themes.cc theminst.cc decorate.cc browse.cc
    objbar.cc objbutton.cc objmenu.cc wmdock.cc
    wmmenu.cc wmprog.cc wmpref.cc atasks.cc aworkspaces.cc
    amailbox.cc aclock.cc acpustatus.cc amemstatus.cc
    applet.cc apppstatus.cc aaddressbar.cc
    akeyboard.cc aapm.cc atray.cc ysmapp.cc yxtray.cc
    )
do_auto_inc_headers(ICEWM_SRCS)
concat_dedup(icewm_libs itk ice ${sm_LIBS} ${fontconfig_LDFLAGS} ${fribidi_LDFLAGS}
                       ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS}
                       ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS} ${icewm_img_libs})

ADD_EXECUTABLE(icewm${EXEEXT} ${ICEWM_SRCS})
TARGET_LINK_LIBRARIES(icewm${EXEEXT} ${icewm_libs})

ADD_EXECUTABLE(genpref${EXEEXT} genpref.cc)
TARGET_LINK_LIBRARIES(genpref${EXEEXT} ice ${x11_LDFLAGS} ${nls_LIBS})

if(BUILD_TESTING)
    ADD_EXECUTABLE(strtest strtest.cc)
    TARGET_LINK_LIBRARIES(strtest ice ${nls_LIBS})
    add_test(strtest ${CMAKE_BINARY_DIR}/strtest)

    ADD_EXECUTABLE(testpointer testpointer.cc)
    target_compile_options(testpointer PUBLIC ${CXXFLAGS_COMMON})
    TARGET_LINK_LIBRARIES(testpointer)
    add_test(testpointer ${CMAKE_BINARY_DIR}/testpointer)

    ADD_EXECUTABLE(testarray testarray.cc)
    TARGET_LINK_LIBRARIES(testarray ice)
    add_test(testarray ${CMAKE_BINARY_DIR}/testarray)
endif()

IF(CONFIG_FDO_MENUS)
    ADD_EXECUTABLE(icewm-menu-fdo${EXEEXT} fdomenu.cc ycollections.cc)
    target_compile_options(icewm-menu-fdo${EXEEXT} PUBLIC ${gio_CFLAGS_OTHER} ${gio_unix_CFLAGS_OTHER})
    target_include_directories(icewm-menu-fdo${EXEEXT} PUBLIC ${gio_INCLUDE_DIRS} ${gio_unix_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(icewm-menu-fdo${EXEEXT} ice ${x11_LDFLAGS}
                          ${gio_LDFLAGS} ${nls_LIBS})
    INSTALL(TARGETS icewm-menu-fdo${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

add_executable(icewm-session${EXEEXT} icesm.cc)
target_link_libraries(icewm-session${EXEEXT} ice ${nls_LIBS} ${x11_LDFLAGS})

add_executable(icewmhint${EXEEXT} icewmhint.cc)
target_link_libraries(icewmhint${EXEEXT} ice ${xext_LDFLAGS} ${x11_LDFLAGS}
                      ${nls_LIBS})

ADD_EXECUTABLE(icesh${EXEEXT} icesh.cc)
concat_dedup(icesh_libs ice ${xrandr_LDFLAGS} ${xinerama_LDFLAGS}
                      ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS})
TARGET_LINK_LIBRARIES(icesh${EXEEXT} ${icesh_libs})

ADD_EXECUTABLE(icewmbg${EXEEXT} icewmbg.cc)
concat_dedup(icewmbg_libs ice ${icewm_img_libs} ${xft_LDFLAGS}
                      ${fribidi_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS}
                      ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS})
TARGET_LINK_LIBRARIES(icewmbg${EXEEXT} ${icewmbg_libs})

IF(ENABLE_ALSA OR ENABLE_AO OR ENABLE_OSS)
    add_executable(icesound${EXEEXT} icesound.cc)
    target_compile_options(icesound${EXEEXT} PUBLIC ${audio_flags})
    concat_dedup(icesound_libs ice ${xext_LDFLAGS} ${x11_LDFLAGS}
                          ${nls_LIBS} ${audio_libs})
    TARGET_LINK_LIBRARIES(icesound${EXEEXT} ${icesound_libs})
    INSTALL(TARGETS icesound${EXEEXT} DESTINATION ${BINDIR})
ENDIF()

add_executable(icehelp${EXEEXT} icehelp.cc)
concat_dedup(icehelp_libs itk ice ${icewm_img_libs} ${xft_LDFLAGS}
                      ${fribidi_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS}
                      ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS})
TARGET_LINK_LIBRARIES(icehelp${EXEEXT} ${icehelp_libs})
INSTALL(TARGETS icehelp${EXEEXT} DESTINATION ${BINDIR})

IF(CONFIG_EXTERNAL_TRAY)
    add_executable(icewmtray${EXEEXT} icetray.cc)
    concat_dedup(icewmtray_libs itk ice ${icewm_img_libs}
                          ${xft_LDFLAGS} ${xrandr_LDFLAGS} ${xinerama_LDFLAGS}
                          ${xext_LDFLAGS} ${x11_LDFLAGS} ${nls_LIBS})
    target_link_libraries(icewmtray${EXEEXT} ${icewmtray_libs})
ENDIF()

get_directory_property(tgts DIRECTORY . BUILDSYSTEM_TARGETS)
foreach(tgt ${tgts})
    if(EXTRA_LINKER_FLAGS)
        target_link_options(${tgt} PRIVATE ${EXTRA_LINKER_FLAGS})
    endif()
    if(EXTRA_LIBS)
        target_link_libraries(${tgt} ${EXTRA_LIBS})
    endif()
endforeach()


INSTALL(TARGETS icewm${EXEEXT} icesh${EXEEXT} icewm-session${EXEEXT}
                icewmhint${EXEEXT} icewmbg${EXEEXT} DESTINATION ${BINDIR})

if(NOT CMAKE_CROSSCOMPILING)
    ADD_CUSTOM_COMMAND(TARGET genpref${EXEEXT}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
        COMMAND genpref${EXEEXT} -o ${CMAKE_BINARY_DIR}/lib/preferences -s
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
        CONFIG_IMLIB2
        CONFIG_LIBRSVG
        CONFIG_XPM
        CONFIG_LIBPNG
        CONFIG_LIBJPEG
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
