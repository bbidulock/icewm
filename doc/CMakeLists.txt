cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

#
# The list of AsciiDoc pages which need to be converted to HTML.
#
set(ADOC_NAMES icewm.adoc)

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
