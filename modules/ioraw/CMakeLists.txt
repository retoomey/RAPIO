# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

add_library(rapioraw SHARED
  rIORaw.cc
)

target_link_libraries(rapioraw
 # We are a module so we don't link to RAPIO itself
 ${RAPIO_THIRD_LIBRARIES}
)

# Install the module
install(TARGETS rapioraw
   LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
