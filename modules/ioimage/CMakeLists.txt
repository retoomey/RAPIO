# Hunt for imagic package
find_package(Magick)
if (Magick_FOUND)

  # We're building a standalone shared library
  addRAPIOModule(rapioimage SHARED
    rImageDataTypeImp.cc
    rIOImage.cc
  )

  # Do some default definitions for ImagicMagick
  target_compile_definitions(rapioimage PRIVATE HAVE_MAGICK)
  target_compile_definitions(rapioimage PRIVATE 
     MAGICKCORE_HDRI_ENABLE=0 
     MAGICKCORE_QUANTUM_DEPTH=16
  )

  # Turn off the deprecation warning since currently the 3rd party
  # library is using deprecated std::unary_function
  target_compile_options(rapioimage PRIVATE "-Wno-deprecated-declarations")

  # Include Magick headers
  target_include_directories(rapioimage PRIVATE ${Magick_INCLUDE_DIR})

  # Include Magick libraries
  target_link_libraries(rapioimage PRIVATE ${Magick_LIBRARIES})

  addRAPIONote("      Found: Imagick")
else()
  addRAPIONote("NO ioimage module: Couldn't find GraphicsMagick or ImageMagick, No rtile or make pngs, etc. without this.")
endif(Magick_FOUND)

