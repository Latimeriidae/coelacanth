#-------------------------------------------------------------------------------
#
# Coelacanth build system -- controlgraph library
#
#-------------------------------------------------------------------------------

set(SRCS
  controlgraph.cc
  splittree.cc
)

add_library(controlgraph STATIC ${SRCS})
target_link_libraries(controlgraph Boost::graph)
add_clang_format_run(controlgraph ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})