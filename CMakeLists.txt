cmake_minimum_required(VERSION 3.2)

project(coelacanth)

include(clang-format.cmake)

if (DEFINED DEBUG_BUILD)
  set(OPTIMIZATION_LEVEL "-O0 -g3 -ggdb -Wall -Wextra -Werror --std=c++17 -Wfatal-errors")
else()
  set(OPTIMIZATION_LEVEL "-O3 -DNDEBUG -Wall -Wextra -Werror --std=c++17 -Wfatal-errors")
endif()

set(CMAKE_C_FLAGS ${OPTIMIZATION_LEVEL})
set(CMAKE_CXX_FLAGS ${OPTIMIZATION_LEVEL})

set(SUBFOLDERS "config;typegraph;callgraph")
foreach(SUBFOLDER ${SUBFOLDERS})
  file(GLOB_RECURSE SUB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/${SUBFOLDER}/*.cc)
  add_library(${SUBFOLDER}lib STATIC ${SUB_SOURCES})
  target_include_directories(${SUBFOLDER}lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/${SUBFOLDER})
  target_include_directories(${SUBFOLDER}lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

# final executable
add_executable(coelacanth coelacanth.cc)
target_link_libraries(coelacanth configlib callgraphlib typegraphlib boost_program_options)

install(TARGETS coelacanth DESTINATION bin)