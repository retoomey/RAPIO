# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

add_library(rapiogrib SHARED
  rGribDataTypeImp.cc
  rIOGrib.cc
)

target_link_libraries(rapiogrib
  grib2c
)

# Install the module
install(TARGETS rapiogrib
   DESTINATION ${RAPIO_INSTALL_LIB}
)
