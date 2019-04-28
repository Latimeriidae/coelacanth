cmake_minimum_required(VERSION 3.2)

project(coelacanth)

# clang-format run
include(cmake/clang-format.cmake)

# version.h header
include(cmake/git-hash.cmake)

# timestamp.h header
include(cmake/time-stamp.cmake)

# build flags
if (DEFINED DEBUG_BUILD AND DEBUG_BUILD STREQUAL "YES")
  set(OPTIMIZATION_LEVEL "-O0 -g3 -ggdb -Wall -Wextra -Werror --std=c++17 -Wfatal-errors -I${CMAKE_BINARY_DIR} -pthread")
else()
  set(OPTIMIZATION_LEVEL "-O3 -DNDEBUG -Wall -Wextra -Werror --std=c++17 -Wfatal-errors -I${CMAKE_BINARY_DIR} -pthread")
endif()

set(CMAKE_C_FLAGS ${OPTIMIZATION_LEVEL})
set(CMAKE_CXX_FLAGS ${OPTIMIZATION_LEVEL})

# main libraries build
file(GLOB SUBFOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/lib/*)
set(LIBLIST "")
foreach(SUBFOLDER ${SUBFOLDERS})
  file(GLOB_RECURSE SUB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/${SUBFOLDER}/*.cc)
  add_library(${SUBFOLDER}lib STATIC ${SUB_SOURCES})
  add_dependencies (${SUBFOLDER}lib clangformat)
  list(APPEND LIBLIST ${SUBFOLDER}lib)
  target_include_directories(${SUBFOLDER}lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/${SUBFOLDER})
  target_include_directories(${SUBFOLDER}lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

# final executable
add_executable(coelacanth coelacanth.cc)
target_link_libraries(coelacanth ${LIBLIST} boost_program_options)
add_dependencies (coelacanth timestamp)

install(TARGETS coelacanth DESTINATION bin)
