# SPDX-FileCopyrightText: Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)
option(CLEAN_TRS "Clean obsolete translations from tr files" OFF)
find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE COMPONENTS LinguistTools)

# Keep in lockstep with kUpstreamId / I18N filters (U-18). Value is captured in
# the root CMakeLists.txt as UPSTREAM_PROJECT_NAME before Synergy.cmake renames
# the project - today that is "deskflow". Renaming .ts files without updating
# those two sites silently disables translations.
set(filename "${UPSTREAM_PROJECT_NAME}")

# To add a new language Add 639 shortname
set (translations
  ${filename}_es.ts
  ${filename}_it.ts
  ${filename}_ja.ts
  ${filename}_zh_CN.ts
  ${filename}_ru.ts
  ${filename}_ko.ts
)

# Trees scanned for translatable strings.
#
# The Synergy overlay under extra/src carries user-visible strings of its own
# (the settings scope tab, the About dialog). lupdate only looks where it is
# told, so omitting it left those strings English in every language while the
# .ts files still reported themselves complete - an easy thing to miss.
set(TR_SOURCE_DIRS
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/extra/src
)

set(TR_OPTIONS -no-ui-lines -locations none -silent)
if(CLEAN_TRS)
  list(APPEND TR_OPTIONS -no-obsolete)
endif()

# English will have only plurals
qt_create_translation(TRS ${TR_SOURCE_DIRS} ${filename}_en.ts OPTIONS -pluralonly ${TR_OPTIONS})

# Other languages contain the full set of strings.
qt_create_translation(TRS ${TR_SOURCE_DIRS} ${translations} OPTIONS ${TR_OPTIONS})

# Qt ships its own strings (standard dialog buttons such as Save/Cancel, built-in
# widget and file-dialog text) as qtbase_<lang>.qm, separately from the
# application's catalogues; I18N loads them as qt_<lang>.qm. Nothing on Windows or
# Linux copied them, so Qt's UI stayed English there even when the application was
# translated.
#
# Locate them first. Two layouts occur: vcpkg installs them under
# <installed>/<triplet>/translations/Qt6, while a stock Qt keeps them relative to
# the tools (derived here from lupdate's own location). They are then copied into
# the build directory under the names I18N expects, so they travel with the
# application catalogues.
get_target_property(lupdate_executable Qt6::lupdate IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${lupdate_executable}" DIRECTORY)
file(REAL_PATH "${_qt_bin_dir}/../" QT_ROOT_DIR)

# System Qt (distro packages) puts catalogues under /usr/share/qt6/translations,
# which is neither under QT_ROOT_DIR (derived from lupdate's bin/) nor covered by
# the old suffixes - configure failed on every Linux CI leg with
# "Could not find qtbase_en.qm". Search /usr and /usr/local explicitly as well.
find_file(
  _QT_QM_FILE
  NAMES qtbase_en.qm
  PATHS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    "${QT_ROOT_DIR}"
    "/usr"
    "/usr/local"
  PATH_SUFFIXES
    "translations/Qt6"
    "translations"
    "share/qt/translations"
    "share/qt6/translations"
  REQUIRED
)
get_filename_component(QT_LANG_PATH "${_QT_QM_FILE}" DIRECTORY)

set(QT_TRANSLATION_FILES)
set(_qt_langs ${translations})
list(APPEND _qt_langs ${filename}_en.ts)
foreach(_ts IN LISTS _qt_langs)
  string(REPLACE "${filename}_" "" _lang "${_ts}")
  string(REPLACE ".ts" "" _lang "${_lang}")
  set(_qt_qm "${CMAKE_CURRENT_BINARY_DIR}/qt_${_lang}.qm")
  add_custom_command(
    OUTPUT "${_qt_qm}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_LANG_PATH}/qtbase_${_lang}.qm" "${_qt_qm}"
    DEPENDS "${QT_LANG_PATH}/qtbase_${_lang}.qm"
    COMMENT "Copying Qt translations for ${_lang}"
    VERBATIM
  )
  list(APPEND QT_TRANSLATION_FILES "${_qt_qm}")
endforeach()

# Expose everything the runtime needs to the parent scope so the top-level build
# can deploy it next to the executables. Without this the binaries look for
# <appDir>/translations, find nothing in a plain build tree, and fall back to
# English even on a non-English system.
set(SYNERGY_TRANSLATION_FILES ${TRS} ${QT_TRANSLATION_FILES} PARENT_SCOPE)

#ensure that the targets are built always
add_custom_target(app_translations ALL DEPENDS ${TRS} ${QT_TRANSLATION_FILES})

if (NOT BUILD_OSX_BUNDLE)
  # install our translations
  install(FILES ${TRS} ${QT_TRANSLATION_FILES} DESTINATION ${CMAKE_INSTALL_I18N_DIR})
else()
  #when making the bundle copy our translation into the bundle
  # Target name is CMAKE_PROJECT_NAME (no spaces); PROPER_NAME is display-only.
  set(BUNDLE_TR_DIR $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_NAME}>/Resources/translations)

  add_custom_command(
    TARGET app_translations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_TR_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TRS} ${QT_TRANSLATION_FILES} ${BUNDLE_TR_DIR}
  )
endif()
