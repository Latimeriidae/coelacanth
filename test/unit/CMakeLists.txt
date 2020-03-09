set(BOOST_TEST_LIBS
  Boost::unit_test_framework
  Boost::timer
  )

set(SRCS
  runner.cc
  )

add_executable(unittests_runner ${SRCS})
add_clang_format_run(unittests_runner ${CMAKE_CURRENT_SOURCE_DIR} ${SRCS})
target_link_libraries(unittests_runner ${BOOST_TEST_LIBS})

# Test libraries. Each of them should add itself to list of unittests
# dependencies. See semitree for example.
add_subdirectory(semitree)
