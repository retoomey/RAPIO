# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

find_package(Netcdf)

# Add conditional flag
if (Netcdf_FOUND)

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
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
    # And of course we also use netcdf
    ${Netcdf_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapionetcdf
    LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find Netcdf library, You probably want this to read NSSL data files, if not ignore.")
endif(Netcdf_FOUND)
