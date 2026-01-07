find_package(HDF5)

if (HDF5_FOUND)

  # Module for HDF5
  addRAPIOModule(rapiohdf5 SHARED
    rIOHDF5.cc
    rHDF5Group.cc 
    rHDF5DataSpace.cc
    rHDF5Attribute.cc
    rHDF5DataType.cc
    rHDF5DataSet.cc
    rODIMDataHandler.cc
    rConfigODIMProductInfo.cc
  )

  # Add HDF5 headers
  target_include_directories(rapiohdf5 PRIVATE ${HDF5_INCLUDE_DIRS})

  # Link to HDF5 libraries
  target_link_libraries(rapiohdf5 PRIVATE ${HDF5_LIBRARIES})

  # Definition HDF5
  target_compile_definitions(rapiohdf5 PRIVATE HAVE_HDF5)

  addRAPIONote("      Found: HDF5")
else()
  addRAPIONote("      Missing: HDF5")
endif(HDF5_FOUND)

