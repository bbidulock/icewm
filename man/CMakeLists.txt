#
# The destination directory for manpages.
#
if(NOT MANDIR)
    set(MANDIR "${DATADIR}/man")
endif()

macro(Pod2Man name sectionId sectionName)
        string(REGEX REPLACE "\\.man$" ".pod" podName "${name}")
        string(REGEX REPLACE "\\.man$" ".${sectionId}" manName "${name}")
        ADD_CUSTOM_COMMAND(
                OUTPUT ${manName}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${podName}
                COMMAND pod2man ARGS --center="${sectionName}"
                --errors="none"
                --fixed="CW"
                --fixedbold="CB"
                --fixeditalic="CI"
                --fixedbolditalic="CX"
                --nourls
                --release="${PACKAGE} ${VERSION}" --section="${sectionId}"
                ${CMAKE_CURRENT_SOURCE_DIR}/${podName} ${manName}
                )
        INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${manName} DESTINATION "${MANDIR}/man${sectionId}")
        list(APPEND MAN_RESULTS ${manName})
endmacro()

set(icewm_man1_files
	icesh.man
	icewmbg.man
	icewmhint.man
	icewm.man
	icewm-session.man
	icewm-set-gnomewm.man
	icewmtray.man
  )

set(extra_man1_files
	icehelp.man
	icesound.man
	icewm-menu-fdo.man
	icewm-menu-xrandr.man
  )

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

foreach(x ${icewm_man1_files} ${extra_man1_files})
        Pod2Man(${x} 1 "User Commands")
endforeach()

foreach(x ${icewm_man5_files})
        Pod2Man(${x} 5 "Standards, Environments and Macros")
endforeach()

ADD_CUSTOM_TARGET(manpages ALL DEPENDS ${MAN_RESULTS})
