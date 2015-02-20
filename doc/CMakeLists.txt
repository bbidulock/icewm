cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

find_program(sgml2html sgml2html)
if(sgml2html)
   ADD_CUSTOM_COMMAND(OUTPUT "icewm.html"
      COMMAND ${sgml2html} "${CMAKE_CURRENT_SOURCE_DIR}/icewm.sgml"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/icewm.sgml"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
   ADD_CUSTOM_TARGET(build_html_docs DEPENDS "icewm.html")
   add_dependencies(icewm build_html_docs)

   if(NOT DEFINED(HTMLDIR))
      if(ICEHELPIDX)
         get_filename_component(HTMLDIR "${ICEHELPIDX}" PATH)
         # message("got path from idx, ${HTMLDIR} from ${ICEHELPIDX}")
      else()
         set(HTMLDIR ${DOCDIR})
      endif()
   endif()
   install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DESTINATION ${DOCDIR}
      FILES_MATCHING PATTERN "*.html" PATTERN "CMake*" EXCLUDE)

else()
   message(WARNING "sgml2html not found, please install linuxdoc tools")
endif()

install(FILES icewm.man DESTINATION ${LIBDIR}/man/man1 RENAME icewm.1)
