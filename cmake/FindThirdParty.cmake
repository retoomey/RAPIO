##################################################################################################
# Toomey Sept 2021
#
# Helper script for 'simple' searching of libraries in Linux.
#
function(find_third_party the_package)
  cmake_parse_arguments(
    PARSE_ARGV 1 FIND_THIRD "REQUIRED" "" "HEADER;LIBRARY;HEADER_PATHS;LIBRARY_PATHS")
  #message(STATUS "Hunting for package=${the_package}")

  find_path(${the_package}_INCLUDE_DIR 
    NO_DEFAULT_PATH 
    NAMES ${FIND_THIRD_HEADER}
    PATHS ${FIND_THIRD_HEADER_PATHS}
  )
  find_library(${the_package}_LIBRARY 
    NO_DEFAULT_PATH
    NAMES ${FIND_THIRD_LIBRARY}
    PATHS ${FIND_THIRD_LIBRARY_PATHS}
  )

  INCLUDE(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(${the_package} DEFAULT_MSG ${the_package}_LIBRARY ${the_package}_INCLUDE_DIR)

  # If found, set _FOUND and the _LIBRARIES 
  if (${the_package}_FOUND)
    #message(STATUS ">>>>>>>>>>>>>>The library ${the_package} was FOUND")
    set(${the_package}_FOUND PARENT_SCOPE)
    set(${the_package}_LIBRARIES ${${the_package}_LIBRARY} PARENT_SCOPE)
  else()
    if (${FIND_THIRD_REQUIRED})
      message(FATAL_ERROR "Couldn't find required package ${the_package}")
    else()
      message(STATUS ">>>>>>>>>>>>>>>>>>>>>>Didn't find package ${the_package}, some stuff might not work.")
    endif()
  endif()
endfunction()
