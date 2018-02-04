ADD_CUSTOM_TARGET(man ALL)

#
# The destination directory for manpages.
#
if(NOT MANDIR)
    set(MANDIR "${DATADIR}/man")
endif()

macro(Pod2Man name sectionId sectionName)
        #set(manName "${name}.${sectionId}")
        #set(podName "${name}")
        string(REGEX REPLACE "\\.man$" ".pod" podName "${name}")
        string(REGEX REPLACE "\\.man$" ".${sectionId}" manName "${name}")
        ADD_CUSTOM_COMMAND(
                TARGET man
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${podName}
                COMMAND pod2man ARGS --center="${sectionName}"
                --errors="none"
                --fixed="CW"
                --fixedbold="CB"
                --fixeditalic="CI"
                --fixedbolditalic="CX"
                --nourls
                --release="${PACKAGE} ${VERSION}" --section="${sectionId}"
                ${CMAKE_CURRENT_SOURCE_DIR}/${podName} ${CMAKE_CURRENT_BINARY_DIR}/${manName}
                OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/${manName})
        ADD_CUSTOM_COMMAND(TARGET man DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${manName})
        INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${manName} DESTINATION "${MANDIR}/man${sectionId}")
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

##foreach(src ${SRCS})
##        string(REGEX REPLACE "\\.pod$" ".1" htm "${adoc}")
##        set(txt "${CMAKE_SOURCE_DIR}/doc/${adoc}")
##        set(out "${CMAKE_BINARY_DIR}/${htm}")
##        ADD_CUSTOM_COMMAND(OUTPUT "${out}"
##            COMMAND "${ASCIIDOC}" "${ASCIIDOC_BACKEND}" -o "${out}" "${txt}"
##            DEPENDS "${txt}"
##            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
##            )
##        list(APPEND HTML_OUTPUT_FILES "${out}")
##    endforeach()
##    ADD_CUSTOM_TARGET(build_html_docs ALL DEPENDS ${HTML_OUTPUT_FILES})
##
##ADD_CUSTOM_COMMAND(
##  TARGET man
##  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/myprog.pod
##  COMMAND pod2man ARGS -s 1 -c "myprog manual" ${CMAKE_CURRENT_SOURCE_DIR}/myprog.pod ${CMAKE_CURRENT_BINARY_DIR}/myprog.1
##  OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/myprog.1
##)
##
##ADD_CUSTOM_COMMAND(
##  TARGET man
##  SOURCE man
##  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/myprog.1
##)
##
##INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/myprog.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
