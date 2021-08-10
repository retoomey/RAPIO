# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ../)

add_library(rapiogrib SHARED
  rGribDataTypeImp.cc
  rIOGrib.cc
)

target_link_libraries(rapiogrib
  grib2c
)

# Install the module
install(TARGETS rapiogrib
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
