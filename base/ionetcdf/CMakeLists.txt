# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

# Need this to add the BOOST setting or the rError.cc will complain about v2
add_definitions(-DBOOST_LOG_DYN_LINK)

add_library(rapionetcdf SHARED
  rIONetcdf.cc
  rNetcdfDataGrid.cc
  rNetcdfBinaryTable.cc
  rNetcdfRadialSet.cc
  rNetcdfLatLonGrid.cc
  rNetcdfDataTypeImp.cc
)

target_link_libraries(rapionetcdf
  boost_system
  netcdf
)

# Install the module
install(TARGETS rapionetcdf
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
