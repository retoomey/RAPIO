##################################################################################################
# Toomey Nov 2021
# My simple cmake module for finding jasper
#
# Incoming:
# JASPER_INCLUDEDIR  = root of search
# JASPER_LIBRARYDIR  = root of search
#
# On success sets the following
# JASPER_FOUND       = if the library found
# JASPER_LIBRARY     = full path to the library
# JASPER_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  Jasper
  REQUIRED
  HEADER jas_init.h
  LIBRARY jasper
  HEADER_PATHS ${Jasper_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include/jasper /usr/include/jasper
  LIBRARY_PATHS ${Jasper_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64
)
