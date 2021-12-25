##################################################################################################
# Toomey Aug 2021
# My simple cmake module for finding UDUNITS2
#
# Incoming:
# UDUnits2_INCLUDEDIR  = root of search
# UDUnits2_LIBRARYDIR  = root of search
#
# On success sets the following
# UDUnits2_FOUND       = if the library found
# UDUnits2_LIBRARY     = full path to the library
# UDUnits2_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  UDUnits2
  HEADER udunits2.h
  LIBRARY udunits2
  HEADER_PATHS ${UDUnits2_INCLUDEDIR} ${UDUnits2_INCLUDEDIR}/udunits2 ${CUSTOM_THIRDDIR}/include/ /usr/include/udunits2
  LIBRARY_PATHS ${UDUnits2_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib
)
