##################################################################################################
# Toomey Aug 2021
# My simple cmake module for finding ImagickMagick OR GraphicsMagick
# I have to deal with older linux vs new where ImageMagick is the older package
#
# Incoming:
# Magick_INCLUDEDIR  = root of search
# Magick_LIBRARYDIR  = root of search
#
# On success sets the following
# Magick_FOUND       = if the library found
# Magick_LIBRARY     = full path to the library
# Magick_INCLUDE_DIR = where to find the headers 

INCLUDE(FindThirdParty)
find_third_party(
  Magick
  #REQUIRED (We handle it missing for now)
  HEADER Magick++.h
  LIBRARY GraphicsMagick++ Magick++-6.Q16
  HEADER_PATHS ${Magick_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include /usr/include/GraphicsMagick /usr/include/ImageMagick
  LIBRARY_PATHS ${Magick_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64
)
