# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

find_package(HDF5)

# Add conditional flag
if (HDF5_FOUND)

  # We're building a standalone shared library
  add_library(rapiohdf5 SHARED
    rIOHDF5.cc
    rHDF5Group.cc 
    rHDF5DataSpace.cc
    rHDF5Attribute.cc
    rHDF5DataType.cc
    rHDF5DataSet.cc
    rODIMDataHandler.cc
    rConfigODIMProductInfo.cc
  )

  # Eh conditional compile of some stuff...probably not needed
  add_definitions(-DHAVE_HDF5)

  # Turn off the deprecation warning since currently the 3rd party
  # library is using deprecated std::unary_function

  # Include headers and libraries
  include_directories(${HDF5_INCLUDE_DIR})
  target_link_libraries(rapiohdf5
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
    # And of course imagic magick or graphic
    ${HDF5_LIBRARIES}
  )

  # Install the module
  install(TARGETS rapiohdf5
   DESTINATION ${RAPIO_INSTALL_LIB}
  )
  addRAPIONote("      Found: HDF5")
else()
  addRAPIONote("      Missing: HDF5")
endif(HDF5_FOUND)

