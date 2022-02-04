# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

add_library(rapiohmrg SHARED
  rIOHmrg.cc
)

target_link_libraries(rapiohmrg
)

# Install the module
install(TARGETS rapiohmrg
   LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
