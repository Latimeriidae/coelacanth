#-------------------------------------------------------------------------------
#
# Coelacanth build system -- timestamp support
#
#-------------------------------------------------------------------------------

file (WRITE ${CMAKE_BINARY_DIR}/timestamp.cmake "STRING(TIMESTAMP TIMEZ UTC)\n")
file (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(WRITE timestamp.h \"#pragma once\\n\\n\")\n")
file (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(APPEND timestamp.h \"#define TIMESTAMP \\\"\${TIMEZ}\\\"\\n\\n\")\n")
add_custom_target (
  timestamp
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/timestamp.cmake
  add_dependencies ${CMAKE_BINARY_DIR}/timestamp.cmake
)
