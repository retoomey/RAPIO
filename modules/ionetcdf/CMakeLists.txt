# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

if (BUILD_MRMS)
  find_package(Netcdf REQUIRED)
else()
  find_package(Netcdf QUIET)
endif(BUILD_MRMS)

# Add conditional flag
if (Netcdf_FOUND)

  add_library(rapionetcdf SHARED
    rIONetcdf.cc
    rNetcdfDataGrid.cc
    rNetcdfBinaryTable.cc
    rNetcdfRadialSet.cc
    rNetcdfLatLonGrid.cc
    rNetcdfLatLonHeightGrid.cc
    rNetcdfDataTypeImp.cc
  )

  # Include headers and libraries
  include_directories(${Netcdf_INCLUDE_DIR})
  target_link_libraries(rapionetcdf
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
    # And of course we also use netcdf
    ${Netcdf_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapionetcdf
    LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
  )
  addRAPIONote("      Found: NETCDF")
else()
  addRAPIONote("      Missing: NETCDF")
endif(Netcdf_FOUND)
