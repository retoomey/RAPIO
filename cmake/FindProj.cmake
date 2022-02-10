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

# TODO: Think I need to do some conditional PROJ4 vs PROJ6 stuff 'maybe'
# This issue is I've migrated RAPIO to 6 already, but lots of OS and MRMS, etc.
# are still on PROJ4 and need migration or newer OS.  I'll have to handle it
# as part of the cmake migration of MRMS.
# Basically proj_api.h vs proj.h and the way of coding things:
# http://proj.org/development/migration.html

INCLUDE(FindThirdParty)
find_third_party(
  Proj
  HEADER proj.h
  LIBRARY proj
  # Ubuntu: libproj-dev /usr/include /usr/lib/x86_64-linux-gnu
  # Fedora: proj-devel /usr/include/ /usr/lib64
  HEADER_PATHS ${Proj_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include /usr/include
  LIBRARY_PATHS ${Proj_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu
)
