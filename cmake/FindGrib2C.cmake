##################################################################################################
# Toomey Nov 2021
# My simple cmake module for finding Grib2C
#
# Incoming:
# Grib2C_INCLUDEDIR  = root of search
# Grib2C_LIBRARYDIR  = root of search
#
# On success sets the following
# Grib2C_FOUND       = if the library found
# Grib2C_LIBRARY     = full path to the library
# Grib2C_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  Grib2C
  HEADER grib2.h
  # g2clib adds versions to the lib name a non standard thing so this will continue to cause
  # some issues probably.
  # Oracle 8/bigbang version g2c_v1.6.0
  LIBRARY grib2c g2c_v1.6.3 g2c_v1.6.0
  HEADER_PATHS ${Grib2C_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include /usr/include
  LIBRARY_PATHS ${Grib2C_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib
)
