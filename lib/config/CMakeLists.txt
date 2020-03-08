#-------------------------------------------------------------------------------
#
# Coelacanth build system -- config library
#
#-------------------------------------------------------------------------------

set(SRCS
  configs.cc
)

add_library(config STATIC ${SRCS})
target_link_libraries(config Boost::program_options)
add_clang_format_run(config ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})