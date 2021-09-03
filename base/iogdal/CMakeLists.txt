# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

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
