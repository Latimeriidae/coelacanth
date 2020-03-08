#-------------------------------------------------------------------------------
#
# Coelacanth build system -- callgraph library
#
#-------------------------------------------------------------------------------

set(SRCS
  callgraph.cc
)

add_library(callgraph STATIC ${SRCS})
target_link_libraries(callgraph Boost::graph)
add_clang_format_run(callgraph ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})