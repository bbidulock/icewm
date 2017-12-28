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
# Install HTML pages.
#
foreach(html ${HTML_OUTPUT_FILES})
    install(FILES "${html}" DESTINATION "${HTMLDIR}")
endforeach()
message(STATUS "HTMLDIR => ${HTMLDIR}")
