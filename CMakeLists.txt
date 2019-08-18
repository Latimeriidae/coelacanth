cmake_minimum_required(VERSION 3.2)

project(coelacanth)

# clang-format run
include(cmake/clang-format.cmake)

# version.h header
include(cmake/git-hash.cmake)

# timestamp.h header
include(cmake/time-stamp.cmake)

# boost support
include(cmake/find-boost.cmake)

# build flags
if (DEFINED UNIX OR DEFINED MINGW)
  message(STATUS "Configuring for Unix or MinGW")
  set(COMMON_FLAGS "-Wall -Wextra -Wfatal-errors --std=c++17 -pthread")
  if (DEFINED DEBUG_BUILD AND DEBUG_BUILD STREQUAL "YES")
    set(OPTIMIZATION_LEVEL "-O0 -g3 -ggdb")
  else()
    set(OPTIMIZATION_LEVEL "-O3 -DNDEBUG")
  endif()
elseif (DEFINED MSVC)
  message(STATUS "Configuring for Visual Studio")
  set(COMMON_FLAGS "/W2 /std:c++17 /EHsc")
  if (DEFINED DEBUG_BUILD AND DEBUG_BUILD STREQUAL "YES")
    set(OPTIMIZATION_LEVEL "/O0 /DEBUG")
  else()
    set(OPTIMIZATION_LEVEL "/O2 /D \"NDEBUG\"")
  endif()
else()
  message(ERROR "Unknown system")
endif()

include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "${OPTIMIZATION_LEVEL} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${OPTIMIZATION_LEVEL} ${COMMON_FLAGS}")

# main libraries build
file(GLOB SUBFOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/lib/*)
set(LIBLIST "")
foreach(SUBFOLDER ${SUBFOLDERS})
  file(GLOB_RECURSE SUB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/${SUBFOLDER}/*.cc)
  file(GLOB_RECURSE SUB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/${SUBFOLDER}/*.h)
  add_library(${SUBFOLDER}lib STATIC ${SUB_SOURCES} ${SUB_HEADERS})
  add_dependencies (${SUBFOLDER}lib clangformat)
  list(APPEND LIBLIST ${SUBFOLDER}lib)
  target_include_directories(${SUBFOLDER}lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/${SUBFOLDER})
  target_include_directories(${SUBFOLDER}lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

# final executable
add_executable(coelacanth coelacanth.cc tasksystem.h dbgstream.h)
target_link_libraries(coelacanth ${LIBLIST} ${Boost_LIBRARIES})
add_dependencies (coelacanth timestamp)

install(TARGETS coelacanth DESTINATION bin)
