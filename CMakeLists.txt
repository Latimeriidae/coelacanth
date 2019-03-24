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

# configurations
file(GLOB_RECURSE CONFIG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/config/*.cc)
add_library(configlib STATIC ${CONFIG_SOURCES})
target_include_directories(configlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/config)
target_include_directories(configlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# callgraph
file(GLOB_RECURSE CALLGRAPH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/callgraph/*.cc)
add_library(callgraphlib STATIC ${CALLGRAPH_SOURCES})
target_include_directories(callgraphlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/callgraph)
target_include_directories(callgraphlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# final executable
add_executable(coelacanth coelacanth.cc)
target_link_libraries(coelacanth configlib callgraphlib boost_program_options)

install(TARGETS coelacanth DESTINATION bin)