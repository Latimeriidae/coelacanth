#-------------------------------------------------------------------------------
#
# Coelacanth build system -- flags definitions
#
#-------------------------------------------------------------------------------
#
# This module provides function add_clang_format_run
# use: add_clang_format_run(target, prefix, list of source files)
# also you may specify SKIP_CLANG_FORMAT=1 on cmake to skip this functionality
#
#-------------------------------------------------------------------------------

if(NOT SKIP_CLANG_FORMAT)
  find_program(CLANG_FORMAT clang-format HINTS ${CLANG_FORMAT_PATH})

  if (NOT CLANG_FORMAT)
     message(STATUS "Probably need to specify CLANG_FORMAT_PATH")
     message(FATAL_ERROR "Clang-format not found")
  endif()
endif()

function(add_clang_format_run TGT PRFX SRCLIST)
  if(SKIP_CLANG_FORMAT)
    return()
  endif()
  set(CFNAME "clangformat_${TGT}")
  set(SRCS "")
  foreach(SRCFL ${SRCLIST})
    list(APPEND SRCS "${PRFX}/${SRCFL}")
  endforeach()
  set(INCDIR ${CMAKE_SOURCE_DIR}/include/${TGT})
  file(GLOB HEADERSRCS ${INCDIR}/*)
  foreach(SRCFL ${HEADERSRCS})
    list(APPEND SRCS "${SRCFL}")
  endforeach()
  add_custom_target(
    ${CFNAME}
    COMMAND ${CLANG_FORMAT}
    -i
    ${SRCS}
  )
  add_dependencies(${TGT} ${CFNAME})
endfunction(add_clang_format_run)
