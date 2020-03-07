#-------------------------------------------------------------------------------
#
# Coelacanth build system -- final executable
#
#-------------------------------------------------------------------------------

set(SRCS
  coelacanth.cc
  dbgstream.cc
  tasksystem.cc
)

add_executable(coelacanth ${SRCS})
target_include_directories(coelacanth PRIVATE ${CMAKE_BINARY_DIR})
add_clang_format_run(coelacanth ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})