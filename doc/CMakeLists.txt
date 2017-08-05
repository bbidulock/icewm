cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

#
# The list of manual pages which need to be generated from AsciiDoc.
#
set(MAN_NAMES icewm.1 icewmbg.1)

#
# The list of AsciiDoc pages which need to be converted to HTML.
#
set(ADOC_NAMES icewm.1.adoc icewmbg.1.adoc icewm.adoc)

#
# The destination directory for HTML pages.
#
if(NOT HTMLDIR)
    if(ICEHELPIDX)
        get_filename_component(HTMLDIR "${ICEHELPIDX}" PATH)
    else()
        set(HTMLDIR "${DOCDIR}")
    endif()
endif()

#
# The destination directory for manpages.
#
if(NOT MANDIR)
    set(MANDIR "${DATADIR}/man")
endif()
set(MAN1DIR "${MANDIR}/man1")

#
# Detect AsciiDoc convertor to HTML.
#
if(NOT ASCIIDOC)
    find_program(ASCIIDOC NAMES asciidoctor asciidoc)
endif()
if((ASCIIDOC) AND (NOT ASCIIDOC_BACKEND))
    if(ASCIIDOC MATCHES asciidoctor$)
        set(ASCIIDOC_BACKEND "--backend=html5")
    elseif(ASCIIDOC MATCHES asciidoc$)
        set(ASCIIDOC_BACKEND "--backend=html4")
    endif()
endif()

#
# Detect AsciiDoc converter to manpage.
#
if(NOT ASCIIMAN)
    find_program(ASCIIMAN NAMES asciidoctor a2x)
endif()
if((ASCIIMAN) AND (NOT ASCIIMAN_FORMAT))
    if(ASCIIMAN MATCHES asciidoctor$)
        set(ASCIIMAN_FORMAT "--backend=manpage")
    elseif(ASCIIMAN MATCHES a2x$)
        set(ASCIIMAN_FORMAT "--format=manpage")
    endif()
endif()

#
# Convert AsciiDoc pages to manpage.
#
if((ASCIIMAN) AND (ASCIIMAN_FORMAT))
    set(MAN_OUTPUT_FILES)
    foreach(man ${MAN_NAMES})
        set(txt "${CMAKE_SOURCE_DIR}/doc/${man}.adoc")
        set(out "${CMAKE_BINARY_DIR}/${man}")
	message("man converter => ${ASCIIMAN}")
        if(ASCIIMAN MATCHES a2x$)
            ADD_CUSTOM_COMMAND(OUTPUT "${out}"
                COMMAND "${ASCIIMAN}" "${ASCIIMAN_FORMAT}" "${txt}"
                DEPENDS "${txt}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                )
        else()
            ADD_CUSTOM_COMMAND(OUTPUT "${out}"
                COMMAND "${ASCIIMAN}" "${ASCIIMAN_FORMAT}"
		-o "${out}" "${txt}"
                DEPENDS "${txt}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                )
        endif()
        list(APPEND MAN_OUTPUT_FILES "${out}")
    endforeach()
    ADD_CUSTOM_TARGET(build_manpages ALL DEPENDS ${MAN_OUTPUT_FILES})
else()
    message(WARNING "asciidoctor and a2x not found.\n"
            "NOT regenerating manpages.\n"
            "To rebuild install asciidoctor or asciidoc with a2x.")
    FILE(GLOB MAN_OUTPUT_FILES *.1)
endif()

#
# Install manpages
#
foreach(man ${MAN_OUTPUT_FILES})
    install(FILES "${man}" DESTINATION "${MAN1DIR}")
endforeach()
message(STATUS "MANDIR => ${MANDIR}")

#
# Convert AsciiDoc pages to HTML.
#
if((ASCIIDOC) AND (ASCIIDOC_BACKEND))
    set(HTML_OUTPUT_FILES)
    foreach(adoc ${ADOC_NAMES})
        string(REGEX REPLACE "\\.adoc$" ".html" htm "${adoc}")
        set(txt "${CMAKE_SOURCE_DIR}/doc/${adoc}")
        set(out "${CMAKE_BINARY_DIR}/${htm}")
        ADD_CUSTOM_COMMAND(OUTPUT "${out}"
            COMMAND "${ASCIIDOC}" "${ASCIIDOC_BACKEND}" -o "${out}" "${txt}"
            DEPENDS "${txt}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        list(APPEND HTML_OUTPUT_FILES "${out}")
    endforeach()
    ADD_CUSTOM_TARGET(build_html_docs ALL DEPENDS ${HTML_OUTPUT_FILES})
else()
    message(WARNING "asciidoctor or its backend not found.\n"
            "NOT regenerating HTML documentation.\n"
            "To rebuild install asciidoctor or asciidoc.")
    FILE(GLOB HTML_OUTPUT_FILES *.html)
endif()

#
# Install HTML pages.
#
foreach(html ${HTML_OUTPUT_FILES})
    install(FILES "${html}" DESTINATION "${HTMLDIR}")
endforeach()
message(STATUS "HTMLDIR => ${HTMLDIR}")
