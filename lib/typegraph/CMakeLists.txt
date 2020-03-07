#-------------------------------------------------------------------------------
#
# Coelacanth build system -- typegraph library
#
#-------------------------------------------------------------------------------

set(SRCS
  typegraph.cc
)

add_library(typegraph STATIC ${SRCS})
add_clang_format_run(typegraph ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})