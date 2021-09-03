# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

# We're building a standalone shared library
add_library(rapioimage SHARED
  rImageDataTypeImp.cc
  rIOImage.cc
)

# Add conditional flag
if (Magick_LIBRARIES AND Magick_INCLUDE_DIR)
  add_definitions(-DHAVE_MAGICK)
else(
  message(WARNING "Couldn't find GraphicsMagick or ImageMagick, You should install if you want the rapioimage module to work.")
)
endif(Magick_LIBRARIES AND Magick_INCLUDE_DIR)

# Include headers and libraries
include_directories(${Magick_INCLUDE_DIR})
target_link_libraries(rapioimage
  ${Magick_LIBRARIES}
)

# Install the module
install(TARGETS rapioimage
   DESTINATION ${RAPIO_INSTALL_LIB}
)
