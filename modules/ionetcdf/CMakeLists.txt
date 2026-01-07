# Hunt for netcdf package
if (BUILD_MRMS)
  find_package(Netcdf REQUIRED)
else()
  find_package(Netcdf QUIET)
endif(BUILD_MRMS)

# Add conditional flag
if (Netcdf_FOUND)

  # Define files for the library
  addRAPIOModule(rapionetcdf SHARED
    rIONetcdf.cc
    rNetcdfDataGrid.cc
    rNetcdfBinaryTable.cc
    rNetcdfRadialSet.cc
    rNetcdfLatLonGrid.cc
    rNetcdfLatLonHeightGrid.cc
    rNetcdfDataTypeImp.cc
  )

  # Include current directory headers too and netcdf
  target_include_directories(rapionetcdf PRIVATE ${Netcdf_INCLUDE_DIR})

  # Link to netcdf libraries
  target_link_libraries(rapionetcdf PRIVATE ${Netcdf_LIBRARIES})

  addRAPIONote("      Found: NETCDF")
else()
  addRAPIONote("      Missing: NETCDF")
endif(Netcdf_FOUND)
