cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX C)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR})

IF(NOT DEFINED(PREFIX))
set(PREFIX "/usr/local" CACHE STRING "Target file space")
ENDIF()
# iiieh, neeing to add some guards, since having this nested construct
# interacts badly when changing a variable afterwards cmake also insists on
# overwritting that variables later :-( if not protected
IF(NOT DEFINED(DATADIR))
set(DATADIR "${PREFIX}/share" CACHE STRING "Shared data files directory")
ENDIF()
IF(NOT DEFINED(LIBDIR))
	set(LIBDIR "${DATADIR}/icewm" CACHE STRING "Location of icewm meta data")
ENDIF()
IF(NOT DEFINED(DOCDIR))
	set(DOCDIR "${DATADIR}/doc/icewm" CACHE STRING "Location of documentation files")
ENDIF()
IF(NOT DEFINED(LOCDIR))
	set(LOCDIR "${DATADIR}/locale" CACHE STRING "System locale data directory")
ENDIF()
IF(NOT DEFINED(CFGDIR))
	set(CFGDIR "${PREFIX}/etc/icewm" CACHE STRING "Location of system-wide configuration files")
ENDIF()
IF(NOT DEFINED(ICEHELPIDX))
	set(ICEHELPIDX "${DOCDIR}/icewm.html" CACHE STRING "Location of HTML manual entry page")
ENDIF()
IF(NOT DEFINED(BINDIR))
set(BINDIR "${PREFIX}/bin")
ENDIF()

FILE(READ "${CMAKE_SOURCE_DIR}/VERSION" vfilecontents)
STRING(REGEX REPLACE "\n" ";" vfilecontents "${vfilecontents}")
foreach (line ${vfilecontents})
	STRING(REGEX REPLACE "=" ";" keyval "${line}")
	list(GET keyval 0 _key)
	IF(NOT DEFINED ${_key})
		list(GET keyval 1 ${_key})
	ENDIF()
endforeach()

add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(lib)
add_subdirectory(doc)

install(FILES icewm-set-gnomewm DESTINATION ${BINDIR} PERMISSIONS WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE WORLD_READ OWNER_READ OWNER_WRITE GROUP_READ)

