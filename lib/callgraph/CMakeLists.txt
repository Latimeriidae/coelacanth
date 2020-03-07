#-------------------------------------------------------------------------------
#
# Coelacanth build system -- callgraph library
#
#-------------------------------------------------------------------------------

set(SRCS
  callgraph.cc
)

add_library(callgraph STATIC ${SRCS})
add_clang_format_run(callgraph ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})