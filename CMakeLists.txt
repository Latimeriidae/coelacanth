cmake_minimum_required(VERSION 3.2)

project(coelacanth)

# clang-format run
include(cmake/clang-format.cmake)

# version.h header
include(cmake/git-hash.cmake)

# timestamp.h header
include(cmake/time-stamp.cmake)

# boost
find_package(Boost REQUIRED COMPONENTS graph regex program_options)
include_directories(${Boost_INCLUDE_DIRS})

# build flags
set(COMMON_FLAGS "-Wall -Wextra -Wfatal-errors --std=c++17 -pthread")
if (DEFINED DEBUG_BUILD AND DEBUG_BUILD STREQUAL "YES")
  set(OPTIMIZATION_LEVEL "-O0 -g3 -ggdb")
else()
  set(OPTIMIZATION_LEVEL "-O3 -DNDEBUG")
endif()

include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "${OPTIMIZATION_LEVEL} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${OPTIMIZATION_LEVEL} ${COMMON_FLAGS}")

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
target_link_libraries(coelacanth ${LIBLIST} ${Boost_LIBRARIES})
add_dependencies (coelacanth timestamp)

install(TARGETS coelacanth DESTINATION bin)
