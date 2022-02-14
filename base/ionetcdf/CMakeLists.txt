# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

find_package(Netcdf)

# Add conditional flag
if (Netcdf_FOUND)

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

  # Include headers and libraries
  include_directories(${Netcdf_INCLUDE_DIR})
  target_link_libraries(rapionetcdf
    #boost_system
    ${Boost_LIBRARIES}
    ${Netcdf_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapionetcdf
    LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find Netcdf library, You probably want this to read NSSL data files, if not ignore.")
endif(Netcdf_FOUND)
