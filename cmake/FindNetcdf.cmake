##################################################################################################
# Toomey Aug 2021
# My simple cmake module for finding Netcdf
#
# Incoming:
# Netcdf_INCLUDEDIR  = root of search
# Netcdf_LIBRARYDIR  = root of search
# CUSTOM_THIRDDIR    = root of my third party override folder
#
# On success sets the following
# Netcdf_FOUND       = if the library found
# Netcdf_LIBRARY     = full path to the library
# Netcdf_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  Netcdf
  HEADER netcdf.h
  LIBRARY netcdf
  # Ubuntu: /usr/include /usr/lib/x86_64-linux-gnu
  # Fedora: /usr/include/ /usr/lib64
  HEADER_PATHS ${Netcdf_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include /usr/include
  LIBRARY_PATHS ${Netcdf_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu
)
