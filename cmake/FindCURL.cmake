##################################################################################################
# Toomey Aug 2021
# My simple cmake module for finding CURL
# There exists one but I haven't figured out how to do static install path
# with it.
#
# Incoming:
# CURL_INCLUDEDIR  = root of search
# CURL_LIBRARYDIR  = root of search
#
# On success sets the following
# CURL_FOUND       = if the library found
# CURL_LIBRARY     = full path to the library
# CURL_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  CURL
  REQUIRED
  HEADER curl.h
  LIBRARY curl
  HEADER_PATHS ${CURL_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include/curl /usr/include/curl
  LIBRARY_PATHS ${CURL_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib
)
