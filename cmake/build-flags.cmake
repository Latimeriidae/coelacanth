#-------------------------------------------------------------------------------
#
# Coelacanth build system -- flags definitions
#
#-------------------------------------------------------------------------------
#
# This module is inteneded to bring together all compile options
#
#-------------------------------------------------------------------------------

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG_BUILD>")

if (DEFINED UNIX OR DEFINED MINGW)
  message(STATUS "Configuring for Unix or MinGW")
  add_compile_options(-Wall -Wextra -Werror -Wfatal-errors -pthread)
  add_compile_options(-Wno-deprecated-copy) # workaround for boost 1.70 with gcc 9.2
elseif (DEFINED MSVC)
  message(STATUS "Configuring for Visual Studio")
  add_compile_options(-W4 -EHsc)
else()
  message(ERROR "Unknown system")
endif()
