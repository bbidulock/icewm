#
# The destination directory for manpages.
#
if(NOT MANDIR)
    set(MANDIR ${DATADIR}/man)
endif()

#
# The destination directory for HTML pages.
#
if(NOT HTMLDIR)
    if(ICEHELPIDX)
        get_filename_component(HTMLDIR ${ICEHELPIDX} PATH)
    else()
        set(HTMLDIR ${DOCDIR})
    endif()
endif()

#
# Convert .pod to manpage.
#
macro(Pod2Man name sectionId sectionName)
    string(REGEX REPLACE "\\.man$" ".pod" podName "${name}")
    string(REGEX REPLACE "\\.man$" ".${sectionId}" manName "${name}")
    ADD_CUSTOM_COMMAND(
        COMMENT Convert ${podName} to ${manName}.
        OUTPUT ${manName}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${podName}
        COMMAND pod2man ARGS
        --center=${sectionName}
        --errors="none"
        --fixed="CW"
        --fixedbold="CB"
        --fixeditalic="CI"
        --fixedbolditalic="CX"
        --nourls
        --release="${PACKAGE} ${VERSION}"
        --section="${sectionId}"
        ${CMAKE_CURRENT_SOURCE_DIR}/${podName} ${manName}
        COMMAND sed ARGS -i.bak -r
        -e 's,\([^a-z/]\)/usr/share/icewm,\\1${LIBDIR},'
        -e 's,\([^a-z/]\)/etc/icewm,\\1${CFGDIR},'
        -e 's,\([^A-Z/]\)CFGDIR/,\\1${CFGDIR}/,g'
        -e 's,\([^A-Z/]\)LIBDIR/,\\1${LIBDIR}/,g'
        -e 's,\@CFGDIR\@,${CFGDIR},g'
        ${manName}
        COMMAND rm -f -- ${manName}.bak
        )
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${manName}
            DESTINATION ${MANDIR}/man${sectionId})
    list(APPEND MAN_RESULTS ${manName})
endmacro()

#
# Convert .pod to html.
#
macro(Pod2Html name sectionId sectionName)
    string(REGEX REPLACE "\\.man$" ".pod" podName "${name}")
    string(REGEX REPLACE "\\.man$" ".${sectionId}.html" htmlName "${name}")
    ADD_CUSTOM_COMMAND(
        COMMENT Convert ${podName} to ${htmlName}.
        OUTPUT ${htmlName}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${podName}
        COMMAND pod2html ARGS
        --title=${sectionName}
        --backlink
        --header
        --poderrors
        --podroot=${CMAKE_CURRENT_SOURCE_DIR}
        --podpath=man
        --htmldir=${HTMLDIR}
        --noindex
        --infile=${CMAKE_CURRENT_SOURCE_DIR}/${podName}
        --outfile=${htmlName}
        --quiet
        COMMAND sed ARGS -i.bak -r
        -e 's,http://man.he.net/man\(.\)/\(ice[^\"]*\),\\2.\\1.html,g'
        -e 's,man.he.net/man\(.\)/\([^\"]*\),manned.org/\\2.\\1,g'
        -e 's,\([^a-z/]\)/usr/share/icewm,\\1${LIBDIR},'
        -e 's,\([^a-z/]\)/etc/icewm,\\1${CFGDIR},'
        -e 's,\([^A-Z/]\)CFGDIR/,\\1${CFGDIR}/,g'
        -e 's,\([^A-Z/]\)LIBDIR/,\\1${LIBDIR}/,g'
        -e 's,\@CFGDIR\@,${CFGDIR},g'
        -e 's,<h1,<h2,g'
        -e 's,h1>,h2>,g'
        -e 's,http:,https:,g' ${htmlName}
        COMMAND rm -f -- ${htmlName}.bak
        )
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${htmlName}
            DESTINATION ${HTMLDIR})
    list(APPEND HTML_RESULTS ${htmlName})
endmacro()

set(icewm_man1_files
    icehelp.man
    icesh.man
    icesound.man
    icewm-menu-xrandr.man
    icewm-session.man
    icewm-set-gnomewm.man
    icewm.man
    icewmbg.man
    icewmhint.man
)

if(CONFIG_EXTERNAL_SYSTRAY)
    list(APPEND icewm_man1_files icewmtray.man)
endif()
if(CONFIG_FDO_MENUS)
    list(APPEND icewm_man1_files icewm-menu-fdo.man)
endif()

set(icewm_man5_files
    icewm-env.man
    icewm-focus_mode.man
    icewm-keys.man
    icewm-menu.man
    icewm-preferences.man
    icewm-prefoverride.man
    icewm-programs.man
    icewm-shutdown.man
    icewm-startup.man
    icewm-theme.man
    icewm-toolbar.man
    icewm-winoptions.man
)

foreach(x ${icewm_man1_files})
        Pod2Man(${x} 1 "User Commands")
        Pod2Html(${x} 1 "User Commands")
endforeach()

foreach(x ${icewm_man5_files})
        Pod2Man(${x} 5 "Standards, Environments and Macros")
        Pod2Html(${x} 5 "Standards, Environments and Macros")
endforeach()

ADD_CUSTOM_TARGET(manpages ALL DEPENDS ${MAN_RESULTS} ${HTML_RESULTS})
