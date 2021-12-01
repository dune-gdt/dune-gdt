macro(dune_xt_module_version_from_git TARGET_MODULE)

  dune_module_to_uppercase(TARGET_MODULE_UPPER ${TARGET_MODULE})

  if(dune-xt_MODULE_PATH)
    set(VERSIONEER_DIR ${dune-xt_MODULE_PATH})
  else()
    set(VERSIONEER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
  endif()

  if(${TARGET_MODULE}_SOURCE_DIR)
    # the "self" case
    set(_MODULE_SOURCE_DIR ${${TARGET_MODULE}_SOURCE_DIR})
  else()
    # the "other module" case
    set(_MODULE_SOURCE_DIR ${${TARGET_MODULE}_PPREFIX})
  endif()

  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${VERSIONEER_DIR}/versioneer.py ${_MODULE_SOURCE_DIR}
    WORKING_DIRECTORY ${_MODULE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(GIT_DESCRIBE_ERROR_CODE)
    message(FATAL_ERROR "Extracting version information failed: ${GIT_DESCRIBE_ERROR}")
  endif()

  foreach(_MOD_VAR ${TARGET_MODULE} ${TARGET_MODULE_UPPER})
    set(${_MOD_VAR}_VERSION ${GIT_DESCRIBE_VERSION})
    # Reset variables from dune-common/cmake/modules/DuneMacros.cmake:dune_module_information
    extract_major_minor_version("${GIT_DESCRIBE_VERSION}" DUNE_VERSION)
    set(${_MOD_VAR}_VERSION_MAJOR "${DUNE_VERSION_MAJOR}")
    set(${_MOD_VAR}_VERSION_MINOR "${DUNE_VERSION_MINOR}")
    set(${_MOD_VAR}_VERSION_REVISION "${DUNE_VERSION_REVISION}")
  endforeach(_MOD_VAR)
  set(CPACK_PACKAGE_NAME "${DUNE_MOD_NAME}")
  set(CPACK_PACKAGE_VERSION "${DUNE_VERSION_MAJOR}.${DUNE_VERSION_MINOR}.${DUNE_VERSION_REVISION}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
  set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}" "\\\\.svn" "\\\\.git" ".*/*\\\\.gitignore")

  # reset variables from dune-common/cmake/modules/DuneMacros.cmake:dune_project
  set(ProjectVersion "${GIT_DESCRIBE_VERSION}")
  set(ProjectVersionString "${DUNE_VERSION_MAJOR}.${DUNE_VERSION_MINOR}.${DUNE_VERSION_REVISION}")
  set(ProjectVersionMajor "${DUNE_VERSION_MAJOR}")
  set(ProjectVersionMinor "${DUNE_VERSION_MINOR}")
  set(ProjectVersionRevision "${DUNE_VERSION_REVISION}")

  # need to re-run template insertion
  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/${ProjectName}-config-version.cmake.in)
    file(
      WRITE ${PROJECT_BINARY_DIR}/CMakeFiles/${ProjectName}-config-version.cmake.in
      "set(PACKAGE_VERSION \"${ProjectVersionString}\")

  if(\"\${PACKAGE_FIND_VERSION_MAJOR}\" EQUAL \"${ProjectVersionMajor}\" AND
       \"\${PACKAGE_FIND_VERSION_MINOR}\" EQUAL \"${ProjectVersionMinor}\")
    set (PACKAGE_VERSION_COMPATIBLE 1) # compatible with newer
    if (\"\${PACKAGE_FIND_VERSION}\" VERSION_EQUAL \"${ProjectVersionString}\")
      set(PACKAGE_VERSION_EXACT 1) #exact match for this version
    endif()
  endif()
  ")
    set(CONFIG_VERSION_FILE ${PROJECT_BINARY_DIR}/CMakeFiles/${ProjectName}-config-version.cmake.in)
  else()
    set(CONFIG_VERSION_FILE ${PROJECT_SOURCE_DIR}/${ProjectName}-config-version.cmake.in)
  endif()
  configure_file(${CONFIG_VERSION_FILE} ${PROJECT_BINARY_DIR}/${ProjectName}-config-version.cmake @ONLY)

endmacro(dune_xt_module_version_from_git)
