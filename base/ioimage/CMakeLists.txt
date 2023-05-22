# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

find_package(Magick)

# Add conditional flag
if (Magick_FOUND)

  # Do some default definitions for ImagicMagick
  add_definitions(-DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16)

  # We're building a standalone shared library
  add_library(rapioimage SHARED
    rImageDataTypeImp.cc
    rIOImage.cc
  )

  # Eh conditional compile of some stuff...probably not needed
  add_definitions(-DHAVE_MAGICK)

  # Turn off the deprecation warning since currently the 3rd party
  # library is using deprecated std::unary_function
  target_compile_options(rapioimage PRIVATE "-Wno-deprecated-declarations")

  # Include headers and libraries
  include_directories(${Magick_INCLUDE_DIR})
  target_link_libraries(rapioimage
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
    # And of course imagic magick or graphic
    ${Magick_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapioimage
   DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find GraphicsMagick or ImageMagick, You won't be able to use rtile or make pngs, etc. without this.")
endif(Magick_FOUND)

