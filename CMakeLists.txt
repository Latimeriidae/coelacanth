#-------------------------------------------------------------------------------
#
# Coelacanth build system -- main entry
#
#-------------------------------------------------------------------------------
#
# Structure of build is:
# root
#    /lib        -- for libraries with subsequent cmakes
#    /include    -- for includes
#    /cmake      -- for cmake modules
#    /tools      -- for tools and executables (coelacanth itself is a tool)
#
# You shall use CMAKE_BUILD_TYPE = DEBUG | RELEASE when building
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.14)

project(coelacanth)

set(CMAKE_CXX_STANDARD 17)

# clang-format run
include(cmake/clang-format.cmake)

# version.h header
include(cmake/git-hash.cmake)

# timestamp.h header
include(cmake/time-stamp.cmake)

# boost support
include(cmake/find-boost.cmake)

# build flags
include(cmake/build-flags.cmake)

# collecting libs
add_subdirectory(lib)
message(STATUS "Collected libs: ${LIBLIST}")

# common includes
foreach(LIB ${LIBLIST})
  target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/${LIB})
  target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

# collecting tools
add_subdirectory(tools)
message(STATUS "Collected tools: ${TOOLLIST}")

# link all libraries to every tool -- just in case
foreach(TOOL ${TOOLLIST})
  target_include_directories(${TOOL} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/${TOOL})
  target_include_directories(${TOOL} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(${TOOL} ${LIBLIST} ${Boost_LIBRARIES})
  add_dependencies(${TOOL} timestamp)
  install(TARGETS ${TOOL} DESTINATION bin)
endforeach()
