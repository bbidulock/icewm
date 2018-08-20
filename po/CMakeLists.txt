cmake_minimum_required(VERSION 2.6)
PROJECT(ICEWM CXX)

# yes, FindGettext.cmake exists but it's not really handy :-(
# XXX: document -DEXTRA_MSGMERGE=--verbose -DEXTRA_MSGFMT=--verbose

SET(EXTRA_MSGMERGE "--quiet")

find_program(gettext gettext)
find_program(xgettext xgettext)
find_program(msgmerge msgmerge)
find_program(msgfmt msgfmt)

set(COPYRIGHT_HOLDER "Marko Macek <Marko.Macek@gmx.net>")
set(BUG_ADDRESS "https://github.com/bbidulock/icewm/issues")

if(gettext AND xgettext AND msgmerge AND msgfmt)
	file (STRINGS "POTFILES.in" SRCS)
	# grrr, for make dependency the path needs to match the perspective from the po/ directory.
	# FIXME, how to simplify? Use abs.path for now
	foreach(_src ${SRCS})
		LIST(APPEND SRCSABSOLUTE "${CMAKE_SOURCE_DIR}/${_src}")
	endforeach()
	FILE(GLOB POS "${CMAKE_CURRENT_SOURCE_DIR}/*.po")
	foreach(_src ${POS})
		get_filename_component(_src "${_src}" NAME_WE)
		LIST(APPEND LANGUAGES ${_src})
	endforeach()

	SET(XGETTEXT_CMD xgettext --from-code=UTF-8 -language=CXX --add-comments=TRANSLATORS: --keyword=_ --keyword=N_ -s --package-name=${PACKAGE} --copyright-holder="${COPYRIGHT_HOLDER}"  --package-version="${VERSION}" --msgid-bugs-address="${BUG_ADDRESS}" )
	SET(_potFile "${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE}.pot")

	# XXX: it "escapes" spaces in arguments no matter how they are quoted. Find a workaround!
	ADD_CUSTOM_COMMAND(OUTPUT stamp-potfile
		COMMAND ${XGETTEXT_CMD} -o "${_potFile}" ${SRCS}
		DEPENDS ${SRCSABSOLUTE}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Extracting translatable messages to ${_potFile} -- may require config restart!")
	ADD_CUSTOM_TARGET(update_pot DEPENDS stamp-potfile)

  if(EXISTS ${_potFile})

	if(SKIP_TRANSLATIONS)
		ADD_CUSTOM_TARGET(translations)
	else()
		ADD_CUSTOM_TARGET(translations ALL)
	endif()

	foreach(_lang ${LANGUAGES})
		SET(_tempPO "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.po")
		ADD_CUSTOM_COMMAND(OUTPUT ${_tempPO}
			COMMAND msgmerge -o ${_tempPO} ${EXTRA_MSGMERGE} -s ${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po ${_potFile}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			COMMENT "Updated: ${_tempPO}")
		ADD_CUSTOM_TARGET(${_lang}.po DEPENDS ${_tempPO})

		SET(_tempMO "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo")
		ADD_CUSTOM_COMMAND(OUTPUT ${_tempMO}
			COMMAND msgfmt -o ${_tempMO} ${EXTRA_MSGFMT} ${_tempPO}
			DEPENDS ${_tempPO}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		#COMMENT "Compiled: ${_tempMO}")
		ADD_CUSTOM_TARGET(${_lang}.gmo DEPENDS ${_tempMO})
		install(FILES ${_tempMO} DESTINATION "${LOCDIR}/${_lang}/LC_MESSAGES/" RENAME "${PACKAGE}.mo" OPTIONAL)
		add_dependencies(translations ${_lang}.gmo)
	endforeach()

  endif()

endif()
