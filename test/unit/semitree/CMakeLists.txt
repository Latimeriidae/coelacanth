set(SRCS
  tree_basic.cc
  tree_iteration.cc
  )

# Should be OBJECT because in other case linker
# will delete unused globals and runner will not see
# any tests in this library.
add_library(semitree_unit OBJECT ${SRCS})
add_clang_format_run(semitree_unit ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})

target_include_directories(semitree_unit PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(semitree_unit ${BOOST_TEST_LIBS})
target_link_libraries(unittests_runner semitree_unit)
