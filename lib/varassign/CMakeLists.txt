#-------------------------------------------------------------------------------
#
# Coelacanth build system -- varassign library
#
#-------------------------------------------------------------------------------

set(SRCS
  varassign.cc
)

add_library(varassign STATIC ${SRCS})
add_clang_format_run(varassign ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})