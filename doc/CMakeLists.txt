cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

if(NOT DEFINED(HTMLDIR))
	if(ICEHELPIDX)
		get_filename_component(HTMLDIR "${ICEHELPIDX}" PATH)
	else()
		set(HTMLDIR ${DOCDIR})
	endif()
endif()

find_program(sgml2html sgml2html)
if(sgml2html)
   ADD_CUSTOM_COMMAND(OUTPUT "icewm.html"
      COMMAND ${sgml2html} "${CMAKE_CURRENT_SOURCE_DIR}/icewm.sgml"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/icewm.sgml"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
   ADD_CUSTOM_TARGET(build_html_docs ALL DEPENDS "icewm.html")
   install(
	   CODE "file( GLOB _GeneratedHTMLs \"${CMAKE_CURRENT_BINARY_DIR}/*.html\" )"
	   CODE "file( INSTALL \${_GeneratedHTMLs} DESTINATION \"${HTMLDIR}\" )"
	   )
else()
	message(WARNING "sgml2html not found, will install NOT refreshed documentation. Please install linuxdoc tools to rebuild them.")
	install(
		CODE "file( GLOB _GeneratedHTMLs \"${CMAKE_CURRENT_SOURCE_DIR}/*.html\" )"
		CODE "file( INSTALL \${_GeneratedHTMLs} DESTINATION \"${HTMLDIR}\" )"
		)
endif()
message(STATUS "HTMLDIR => ${HTMLDIR}")
install(FILES icewm.man DESTINATION ${DATADIR}/man/man1 RENAME icewm.1)
