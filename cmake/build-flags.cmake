#-------------------------------------------------------------------------------
#
# Coelacanth build system -- flags definitions
#
#-------------------------------------------------------------------------------
#
# This module is inteneded to bring together all compile options
#
#-------------------------------------------------------------------------------

set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG_BUILD>")

if (DEFINED UNIX OR DEFINED MINGW)
  message(STATUS "Configuring for Unix or MinGW")
  add_compile_options(-Wall -Wextra -Werror -Wfatal-errors)
  add_compile_options(-Wno-deprecated-copy) # workaround for boost 1.70 with gcc 9.2
  add_compile_options(-Wno-array-bounds)    # workaround for GCC madness
elseif (DEFINED MSVC)
  message(STATUS "Configuring for Visual Studio")
  add_compile_options(-W4 -EHsc)
  add_compile_options(-experimental:external -external:W0 -external:anglebrackets)
else()
  message(ERROR "Unknown system")
endif()
