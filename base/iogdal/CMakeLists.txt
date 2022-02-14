# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

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
  ${GDAL_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapiogdal
   DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find GDAL library, You won't be able to write GDAL formats like GeoTiff.")
endif(GDAL_FOUND)
