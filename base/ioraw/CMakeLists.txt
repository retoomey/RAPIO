# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

add_library(rapioraw SHARED
  rIORaw.cc
)

target_link_libraries(rapioraw
)

# Install the module
install(TARGETS rapioraw
   LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
