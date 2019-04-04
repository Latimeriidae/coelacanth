file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.cc *.h *.hpp)

add_custom_target(
  clangformat
  COMMAND clang-format 
  -i
  ${ALL_SOURCE_FILES}
)