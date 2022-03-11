# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

find_package(GDAL)

# Add conditional flag
if (GDAL_FOUND)

  # Build source as a shared library
  add_library(rapiogdal SHARED
    rGDALDataTypeImp.cc
    rIOGDAL.cc
  )

  # Include headers and libraries
  include_directories(${GDAL_INCLUDE_DIR})
  target_link_libraries(rapiogdal
   # We are a module so we don't link to RAPIO itself
   ${RAPIO_THIRD_LIBRARIES}
   # And of course the GDAL libraries
   ${GDAL_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapiogdal
   DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find GDAL library, You won't be able to write GDAL formats like GeoTiff.")
endif(GDAL_FOUND)
