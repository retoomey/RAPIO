# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

add_library(rapiohmrg SHARED
  rIOHmrg.cc
  rHmrgProductInfo.cc
  rHmrgRadialSet.cc
  rHmrgLatLonGrids.cc
)

target_link_libraries(rapiohmrg
  ${RAPIO_INCLUDE_LIBRARIES}
)

# Install the module
install(TARGETS rapiohmrg
   LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
