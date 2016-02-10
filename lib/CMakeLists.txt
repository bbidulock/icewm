cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

foreach(_src keys menu programs toolbar winoptions)
   CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${_src}.in" "${CMAKE_CURRENT_BINARY_DIR}/${_src}")
   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_src} DESTINATION ${LIBDIR})
endforeach()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icewm-session.desktop DESTINATION "${DATADIR}/xsessions/")

install(DIRECTORY mailbox taskbar icons ledclock themes DESTINATION ${LIBDIR} FILES_MATCHING PATTERN "*" PATTERN "*.in" EXCLUDE)

