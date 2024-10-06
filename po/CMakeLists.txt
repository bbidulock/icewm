cmake_minimum_required(VERSION 3.5)
project(ICEWM CXX)

#
# Custom PO file processing (POT extraction/update, PO merging, MO building).
#
# Callable custom targets (part of ALL target):
#
# The default rule set updates pot, *.po, and compiles *.(g)mo from the updated
# versions. Also some CACHED options:
# - with PO_REPLACE options, the updated files are sync'ed back to source code
#   folder in the build phase.
# - with PO_REPLACE_ONCE the original PO/POT files are replaced once and then
#   not again (cached)
# - with SKIP_TRANSLATIONS, the whole PO processing is turned off and MO files
#   are NOT installed. This might be handy in the development process.
#
# when gettext tools are missing, no processing is performed.
#

option(SKIP_TRANSLATIONS "[DEV] Don't perform PO/MO file creation or installation" off)
option(PO_VERBOSE "[DEV] Extra verbosity in gettext processing (extends EXTRA_MSGMERGE and EXTRA_MSGMERGE)" off)
option(PO_REPLACE "[DEV] Install updated POT/PO files in source folder" off)
option(PO_REPLACE_ONCE "[DEV] Like PO_REPLACE but turns itself off after one run" off)

if(SKIP_TRANSLATIONS)
    message(STATUS "Language file processing disabled by SKIP_TRANSLATIONS option")
    return()
endif()

if(PO_VERBOSE)
    list(APPEND EXTRA_MSGMERGE --verbose)
    list(APPEND EXTRA_MSGFMT --verbose)
else()
    list(APPEND EXTRA_MSGMERGE --quiet)
endif()

#
# NOTE: FindGettext.cmake exists but it's not really handy :-(
#
find_program(gettext gettext)
find_program(xgettext xgettext)
find_program(msgmerge msgmerge)
find_program(msgfmt msgfmt)

set(COPYRIGHT_HOLDER "Marko Macek <Marko.Macek@gmx.net>")
set(BUG_ADDRESS "https://github.com/bbidulock/icewm/issues")

if(NOT gettext OR NOT xgettext OR NOT msgmerge OR NOT msgfmt)
    message(STATUS "Some of those tools not found, skipping translations! gettext, xgettext, msgmerge, msgfmt")
    return()
endif()

set(_potFilePublic "${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE}.pot")
set(_potFile "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE}.pot")
set(_potFileStamp "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE}.pot.stamp")

# NOTE: xgettext uses the -f option (and started from project dir so that
# unwanted path information is not leaked) but we also want to track the file
# dependencies, thus building the list here explicitly.
file (STRINGS "POTFILES.in" SRCS_IN)
set(SRCS)
foreach(_src ${SRCS_IN})
    cmake_path(ABSOLUTE_PATH _src BASE_DIRECTORY ${CMAKE_SOURCE_DIR} NORMALIZE)
    list(APPEND SRCS ${_src})
endforeach()

file(GLOB POS "${CMAKE_CURRENT_SOURCE_DIR}/*.po")
foreach(_src ${POS})
    get_filename_component(_src "${_src}" NAME_WE)
    # Ignore junk
    if("${_src}" MATCHES "^[a-z][a-z](_[A-Z][A-Z])?$")
        list(APPEND LANGUAGES ${_src})
    endif()
endforeach()

set(XGETTEXT_CMD xgettext -f po/POTFILES.in --from-code=UTF-8 -language=CXX --add-comments=TRANSLATORS: --keyword=_ --keyword=N_  --package-name=${PACKAGE} --copyright-holder="${COPYRIGHT_HOLDER}"  --package-version="${VERSION}" --msgid-bugs-address="${BUG_ADDRESS}" -o "${_potFile}" )

if(PO_REPLACE OR PO_REPLACE_ONCE)
    set(SRC_UPDATE COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_potFile}" "${_potFilePublic}")
else()
    set(SRC_UPDATE )
endif()

add_custom_command(OUTPUT "${_potFileStamp}"
    DEPENDS ${SRCS}
    BYPRODUCTS "${_potFile}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM

    COMMAND ${XGETTEXT_CMD} 

    ${SRC_UPDATE}

    COMMAND ${CMAKE_COMMAND} -E touch "${_potFileStamp}"

    COMMENT "Extracting translatable messages")

add_custom_target(update_pot DEPENDS "${_potFileStamp}")
#message(VERBOSE "Adding non-default update_pot target to update ${_potFile}")

add_custom_target(translations ALL)

foreach(_lang ${LANGUAGES})
    set(_tempPO "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.po")
    set(_srcPO "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po")
    message(VERBOSE "Generating updated ${_tempPO} from ${_srcPO}")
    add_custom_command(OUTPUT ${_tempPO}
        COMMAND msgmerge -o "${_tempPO}" ${EXTRA_MSGMERGE} -s "${_srcPO}" "${_potFile}"
        #WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po" "${_potFile}" update_pot
        #COMMENT "Updated: ${_tempPO}"
        VERBATIM)
    add_custom_target(generate-${_lang}.po DEPENDS ${_tempPO} ALL)

    if(PO_REPLACE OR PO_REPLACE_ONCE)
        set(SRC_UPDATE COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_tempPO}" "${_srcPO}")
        message(STATUS "Shall replace: ${_srcPO}")
    else()
        set(SRC_UPDATE )
    endif()

    set(_tempMO "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo")
    add_custom_command(OUTPUT "${_tempMO}"
        COMMAND msgfmt -o ${_tempMO} ${EXTRA_MSGFMT} ${_tempPO}
        ${SRC_UPDATE}
        DEPENDS ${_tempPO}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM)
    add_custom_target(generate-${_lang}.gmo DEPENDS ${_tempMO})
    add_dependencies(translations generate-${_lang}.gmo)

    # Install user products 
    install(FILES ${_tempMO} DESTINATION "${LOCDIR}/${_lang}/LC_MESSAGES/" RENAME "${PACKAGE}.mo" OPTIONAL)

    # mimic the content of the installation folder, for development purposes
    set(PG "${CMAKE_CURRENT_BINARY_DIR}/playground")
    file(MAKE_DIRECTORY "${PG}")
    file(MAKE_DIRECTORY "${PG}/${_lang}/LC_MESSAGES/")
    file(CREATE_LINK ${_tempMO} "${PG}/${_lang}/LC_MESSAGES/${PACKAGE}.mo" SYMBOLIC)

endforeach()

if(PO_REPLACE_ONCE)
    unset(PO_REPLACE)
    unset(PO_REPLACE_ONCE)
    unset(PO_REPLACE CACHE)
    unset(PO_REPLACE_ONCE CACHE)
endif()

# vim: ts=4 shiftwidth=4 et
