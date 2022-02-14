# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

find_package(Magick)

# Add conditional flag
if (Magick_FOUND)

  # We're building a standalone shared library
  add_library(rapioimage SHARED
    rImageDataTypeImp.cc
    rIOImage.cc
  )

  # Eh conditional compile of some stuff...probably not needed
  add_definitions(-DHAVE_MAGICK)

  # Include headers and libraries
  include_directories(${Magick_INCLUDE_DIR})
  target_link_libraries(rapioimage
    ${Magick_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapioimage
   DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find GraphicsMagick or ImageMagick, You won't be able to use rtile or make pngs, etc. without this.")
endif(Magick_FOUND)

