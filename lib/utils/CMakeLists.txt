#-------------------------------------------------------------------------------
#
# Coelacanth build system -- utils library
#
#-------------------------------------------------------------------------------

set(SRCS
  indent_ostream.cc
)

add_library(utils STATIC ${SRCS})
target_link_libraries(utils Boost::iostreams)
add_clang_format_run(utils ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})
