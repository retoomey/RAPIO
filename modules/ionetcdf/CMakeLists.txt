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

  # Build tests conditionally
  if(BUILD_TESTS)
    add_executable(rTestNetcdf
      rTestNetcdf.cc
    )
    
    # Link to rapio base
    target_link_libraries(rTestNetcdf PRIVATE rapio)
    
    # Register the test with CTest

    # Run the integration test using the internal Fake Data Generator
    # This actually runs the netcdf from fake data writing to
    # /tmp/NETCDFTEST and speed tests various netcdf options
    add_test(
      NAME rTestNetcdf_Integration
      COMMAND rTestNetcdf 
              -n=disable 
              -r old                 # Force archive mode so FakeIndex dumps records immediately
              -maxcount=10           # Exit successfully after 10 files
              -i fake=KTLX           # Trigger the FakeIndex protocol with KTLX as the radar
              -I=* -o netcdf=/tmp/NETCDFTEST
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Optional: Keep the install if you actually distribute the test binaries
    # install(TARGETS rTestNetcdf DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  addRAPIONote("      Found: NETCDF")
else()
  addRAPIONote("      Missing: NETCDF")
endif(Netcdf_FOUND)
