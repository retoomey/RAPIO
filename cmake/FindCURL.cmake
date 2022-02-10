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
  HEADER curl.h
  LIBRARY curl
  # Ubuntu: libcurl4-gnutls-dev /usr/include/x86_64-linux-gnu/curl /usr/lib/x86_64-linux-gnu
  # Fedora: libcurl-devel /usr/include/curl /usr/lib64
  HEADER_PATHS ${CURL_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include/curl /usr/include/curl /usr/include/x86_64-linux-gnu/curl
  LIBRARY_PATHS ${CURL_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu
)
