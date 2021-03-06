#-------------------------------------------------------------------------------
#
# Coelacanth build system -- looking for boost
#
#-------------------------------------------------------------------------------

# we need to find mt versions
set(Boost_USE_MULTITHREADED ON)

# note: to find it for mingw 8.1, you shall patch lib\cmake\BoostDetectToolset-1.70.0.cmake
find_package(Boost REQUIRED
  COMPONENTS
  graph
  regex
  program_options
  serialization
  unit_test_framework
  timer # needed by unit tests but not imported as a dependency by FindBoost
  iostreams
  )

# at least boost_DIR shall exist
if("${Boost_DIR}" STREQUAL "")
  message(FATAL_ERROR "Boost dir not found")
endif()

if(DEFINED Boost_USE_STATIC_LIBS)
  # Define macro for boost test when using shared libs.
  if(NOT Boost_USE_STATIC_LIBS)
    add_compile_definitions(BOOST_TEST_DYN_LINK)
  endif()
else()
  # By default, windows uses static libs and other systems use shared.
  if(NOT WIN32)
    add_compile_definitions(BOOST_TEST_DYN_LINK)
  endif()
endif()

# add boost include directories
# with strange MinGW bug workaround
if("${Boost_INCLUDE_DIRS}" STREQUAL "")
  SET(Boost_INCLUDE_DIRS "${Boost_DIR}/../../../include/boost-${Boost_VERSION_MAJOR}_${Boost_VERSION_MINOR}")
endif()
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# in MinGW build system imported targets are used instead of Boost_LIBRARIES string 
if("${Boost_LIBRARIES}" STREQUAL "")
  SET(Boost_LIBRARIES
    Boost::graph
    Boost::regex
    Boost::program_options
    Boost::serialization
    Boost::unit_test_framework
    Boost::timer
    Boost::iostreams
    )
endif()

# print all vars after boost was found -- useful for cmake debugging
if(DUMPVARS)
  get_cmake_property(_variableNames VARIABLES)  
  list(SORT _variableNames)
  foreach(_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endif()
