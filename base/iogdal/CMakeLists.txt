# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ../)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

# Build source as a shared library
add_library(rapiogdal SHARED
  rGDALDataTypeImp.cc
  rIOGDAL.cc
)

# Link to other libraries
target_link_libraries(rapiogdal
  gdal
)

# Install the module
install(TARGETS rapiogdal
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
