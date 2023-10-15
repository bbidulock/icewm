cmake_minimum_required(VERSION 3.5)
PROJECT(ICEWM CXX)

#
# The list of files which need to be converted to HTML.
#
set(MARK_FILES icewm.md)
set(ADOC_FILES icewm.adoc)

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
# Detect input processor.
#
if(NOT MARKDOWN AND NOT ASCIIDOC)
    find_program(MARKDOWN NAMES markdown markdown_py discount-markdown Markdown.pl)
    if (NOT MARKDOWN)
        find_program(ASCIIDOC NAMES asciidoctor asciidoc)
    endif()
endif()

#
# Convert Markdown pages to HTML.
#
if(MARKDOWN)
    set(HTML_OUTPUT_FILES)
    foreach(mark ${MARK_FILES})
        string(REGEX REPLACE \\.md$ .html htm ${mark})
        set(txt "${CMAKE_SOURCE_DIR}/doc/${mark}")
        set(out "${CMAKE_BINARY_DIR}/${htm}")
        ADD_CUSTOM_COMMAND(OUTPUT "${out}"
            COMMAND ${MARKDOWN} "${txt}" > "${out}"
            DEPENDS "${txt}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        list(APPEND HTML_OUTPUT_FILES "${out}")
    endforeach()
    ADD_CUSTOM_TARGET(build_html_docs ALL DEPENDS ${HTML_OUTPUT_FILES})

#
# Convert AsciiDoc pages to HTML.
#
elseif(ASCIIDOC)
    if(NOT ASCIIDOC_BACKEND)
        if(ASCIIDOC MATCHES asciidoctor$)
            set(ASCIIDOC_BACKEND --backend=html5)
        elseif(ASCIIDOC MATCHES asciidoc$)
            set(ASCIIDOC_BACKEND --backend=html4)
        endif()
    endif()

    set(HTML_OUTPUT_FILES)
    foreach(adoc ${ADOC_FILES})
        string(REGEX REPLACE \\.adoc$ .html htm ${adoc})
        set(txt "${CMAKE_SOURCE_DIR}/doc/${adoc}")
        set(out "${CMAKE_BINARY_DIR}/${htm}")
        ADD_CUSTOM_COMMAND(OUTPUT "${out}"
            COMMAND ${ASCIIDOC} ${ASCIIDOC_BACKEND} -o "${out}" "${txt}"
            DEPENDS "${txt}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
        list(APPEND HTML_OUTPUT_FILES "${out}")
    endforeach()
    ADD_CUSTOM_TARGET(build_html_docs ALL DEPENDS ${HTML_OUTPUT_FILES})

else()
    message(WARNING "markdown and asciidoctor not found.\n"
            "NOT regenerating HTML documentation.\n"
            "To rebuild install markdown or asciidoctor.")
    FILE(GLOB HTML_OUTPUT_FILES *.html)
endif()

#
# Install HTML pages.
#
foreach(html ${HTML_OUTPUT_FILES})
    install(FILES "${html}" DESTINATION "${HTMLDIR}")
endforeach()
message(STATUS "HTMLDIR => ${HTMLDIR}")
