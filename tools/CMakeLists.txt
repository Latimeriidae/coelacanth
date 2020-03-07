#-------------------------------------------------------------------------------
#
# Coelacanth build system -- tools root
#
#-------------------------------------------------------------------------------

set(TOOLLIST
  coelacanth
)

foreach(TOOL ${TOOLLIST})
  add_subdirectory(${TOOL})
endforeach()

set(TOOLLIST "${TOOLLIST}" PARENT_SCOPE)