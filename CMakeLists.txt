cmake_minimum_required(VERSION 3.2)

project(coelacanth)

# optional clang-format run
# > make clangformat
include(clang-format.cmake)

# build flags
if (DEFINED DEBUG_BUILD)
  set(OPTIMIZATION_LEVEL "-O0 -g3 -ggdb -Wall -Wextra -Werror --std=c++17 -Wfatal-errors -I${CMAKE_BINARY_DIR}")
else()
  set(OPTIMIZATION_LEVEL "-O3 -DNDEBUG -Wall -Wextra -Werror --std=c++17 -Wfatal-errors -I${CMAKE_BINARY_DIR}")
endif()

set(CMAKE_C_FLAGS ${OPTIMIZATION_LEVEL})
set(CMAKE_CXX_FLAGS ${OPTIMIZATION_LEVEL})

# timestamp
file (WRITE ${CMAKE_BINARY_DIR}/timestamp.cmake "STRING(TIMESTAMP TIMEZ UTC)\n")
file (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(WRITE timestamp.h \"#pragma once\\n\\n\")\n")
file (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(APPEND timestamp.h \"#define TIMESTAMP \\\"\${TIMEZ}\\\"\\n\\n\")\n")
add_custom_target (
  timestamp
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/timestamp.cmake
  add_dependencies ${CMAKE_BINARY_DIR}/timestamp.cmake
)

# git hash
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(
  ${CMAKE_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/version.h
)

# main libraries build
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
add_dependencies (coelacanth timestamp)

install(TARGETS coelacanth DESTINATION bin)