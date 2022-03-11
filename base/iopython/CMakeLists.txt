# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})
    
# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

add_library(rapiopython SHARED
  rIOPython.cc
)

target_link_libraries(rapiopython
  # We are a module so we don't link to RAPIO itself
  ${RAPIO_THIRD_LIBRARIES}
  # FIXME: I think we're gonna be some optional Python.h linking here
  # in the near future
)

# Install the module
install(TARGETS rapiopython
   DESTINATION ${RAPIO_INSTALL_LIB}
)
