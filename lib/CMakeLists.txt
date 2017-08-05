cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

#
# Which themes to install in alphabetical order
#
set(themesList CrystalBlue Helix Infadel2 NanoBlue
    default icedesert metal2 motif win95)

foreach(_src keys menu programs toolbar winoptions)
   CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${_src}.in" "${CMAKE_CURRENT_BINARY_DIR}/${_src}")
   install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_src}" DESTINATION "${LIBDIR}")
endforeach()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icewm.desktop" DESTINATION "${DATADIR}/xsessions/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icewm-session.desktop" DESTINATION "${DATADIR}/xsessions/")

install(DIRECTORY mailbox taskbar icons ledclock
        DESTINATION "${LIBDIR}"
        FILES_MATCHING PATTERN "*"
        PATTERN "*.in" EXCLUDE)

foreach(theme ${themesList})
    install(DIRECTORY "themes/${theme}"
            DESTINATION "${LIBDIR}/themes")
endforeach()


