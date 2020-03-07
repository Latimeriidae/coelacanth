#-------------------------------------------------------------------------------
#
# Coelacanth build system -- git hash support
#
#-------------------------------------------------------------------------------

find_program(GIT git HINTS ${GIT_PATH})

if (NOT GIT)
  message(STATUS "Probably need to specify GIT_PATH")
  message(FATAL_ERROR "Git not found")
endif()

execute_process(
  COMMAND ${GIT} log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(
  ${CMAKE_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/version.h
)
