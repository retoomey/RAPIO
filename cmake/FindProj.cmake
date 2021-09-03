##################################################################################################
# Toomey Aug 2021
# My simple cmake module for finding Proj
#
# Incoming:
# Proj_INCLUDEDIR  = root of search
# Proj_LIBRARYDIR  = root of search
#
# On success sets the following
# Proj_FOUND       = if the library found
# Proj_LIBRARY     = full path to the library
# Proj_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  Proj
  REQUIRED
  HEADER proj.h
  LIBRARY proj
  HEADER_PATHS ${Proj_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include
  LIBRARY_PATHS ${Proj_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64
)
