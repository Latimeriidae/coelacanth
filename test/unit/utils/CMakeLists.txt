set(SRCS
  indent_ostream.cc
  )

# Should be OBJECT because in other case linker
# will delete unused globals and runner will not see
# any tests in this library.
add_library(utils_unit OBJECT ${SRCS})
add_clang_format_run(utils_unit ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})

target_include_directories(utils_unit PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(utils_unit ${BOOST_TEST_LIBS} utils)
target_link_libraries(unittests_runner utils_unit)
