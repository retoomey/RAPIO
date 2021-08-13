include_directories(. ../)

# We're building a standalone shared library
add_library(rapioimage SHARED
  rImageDataTypeImp.cc
  rIOImage.cc
)

##################################################################################################
# FIXME: This is supposed to be a module 'find_package' or something.  I'm learning cmake
find_path( MAGICK_HEADERS NAMES Magick++.h
  HINTS ${CMAKE_PREFIX_INSTALL}/include /usr/include/GraphicsMagick /usr/include/ImageMagick
)
find_library( MAGICK_LIBS NAMES GraphicsMagick++ Magick++-6.Q16
  PATHS ${CMAKE_PREFIX_INSTALL}/lib /usr/lib64
)

if (MAGICK_LIBS AND MAGICK_HEADERS)
  add_definitions(-DHAVE_MAGICK)
  include_directories(${MAGICK_HEADERS})
  message(STATUS "Magick++: ${MAGICK_HEADERS} ${MAGICK_LIBS}")
else(
  message(WARNING "Couldn't find GraphicsMagick or ImageMagick, You should install if you want the rapioimage module to work.")
)
endif(MAGICK_LIBS AND MAGICK_HEADERS)

# Link found magic library to code
target_link_libraries(rapioimage
  ${MAGICK_LIBS}
)
# End our ImagicMagick hunt
##################################################################################################
