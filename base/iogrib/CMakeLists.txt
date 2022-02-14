# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Get headers from core above us, or where ever RAPIO headers are
include_directories(../)

find_package(Grib2C)

# Add conditional flag
if (Grib2C_FOUND)

  # Build source as a shared library
  add_library(rapiogrib SHARED
    rGribDataTypeImp.cc
    rIOGrib.cc
  )

  #add_definitions(-DHAVE_GRIB2C)

  # Include headers and libraries
  include_directories(${Grib2C_INCLUDE_DIR})
  target_link_libraries(rapiogrib
  ${Grib2C_LIBRARIES}
  )
  # Install the module
  install(TARGETS rapiogrib
    DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find G2C library, You won't be able to read Grib2 files.")
endif(Grib2C_FOUND)
