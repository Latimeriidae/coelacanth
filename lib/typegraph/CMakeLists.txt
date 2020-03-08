#-------------------------------------------------------------------------------
#
# Coelacanth build system -- typegraph library
#
#-------------------------------------------------------------------------------

set(SRCS
  typegraph.cc
)

add_library(typegraph STATIC ${SRCS})
target_link_libraries(typegraph Boost::graph)
add_clang_format_run(typegraph ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})