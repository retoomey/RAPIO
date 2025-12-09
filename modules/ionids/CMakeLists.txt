# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

add_library(rapionids SHARED
  rIONIDS.cc
  rNIDSRadialSet.cc
  rNIDSUtil.cc
  # NIDS product table (required)
  rConfigNIDSInfo.cc

  # Blocks of Level III
  rBlockMessageHeader.cc
  rBlockProductDesc.cc
  rBlockProductSymbology.cc
  rBlockRadialSet.cc
)

# Include headers and libraries
target_link_libraries(rapionids
  # We are a module so we don't link to RAPIO itself
  ${RAPIO_THIRD_LIBRARIES}
  )

# Install the module
install(TARGETS rapionids
  LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
addRAPIONote("      Building: NIDS LevelIII module")
