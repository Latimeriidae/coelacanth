#-------------------------------------------------------------------------------
#
# Coelacanth build system -- config library
#
#-------------------------------------------------------------------------------

set(SRCS
  configs.cc
)

add_library(config STATIC ${SRCS})
add_clang_format_run(config ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})