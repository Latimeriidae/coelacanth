find_program(CLANG_FORMAT clang-format HINTS ${CLANG_FORMAT_PATH})

if (NOT CLANG_FORMAT)
  message(STATUS "Probably need to specify CLANG_FORMAT_PATH")
  message(FATAL_ERROR "Clang-format not found")
endif()

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.cc *.h *.hpp)

add_custom_target(
  clangformat
  COMMAND ${CLANG_FORMAT}
  -i
  ${ALL_SOURCE_FILES}
)