# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ../)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

# Build source as a shared library
add_library(rapiogdal SHARED
  rGDALDataTypeImp.cc
  rIOGDAL.cc
)

##################################################################################################
# Hunt for GDAL, we prioritize the MRMS third party build
find_path( GDAL_HEADERS NAMES gdal_priv.h
  HINTS ${CMAKE_PREFIX_INSTALL}/include /usr/include/gdal
)
find_library( GDAL_LIBS NAMES gdal
  PATHS ${CMAKE_PREFIX_INSTALL}/lib /usr/lib64
)

if (GDAL_LIBS AND GDAL_HEADERS)
  include_directories(${GDAL_HEADERS})
  message(STATUS "GDAL: ${GDAL_HEADERS} ${GDAL_LIBS}")
else(
  message(FATAL_ERROR "Couldn't find GDAL library.")
)
endif(GDAL_LIBS AND GDAL_HEADERS)
##################################################################################################

# Link to other libraries
target_link_libraries(rapiogdal
  ${GDAL_LIBS}
)

# Install the module
install(TARGETS rapiogdal
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
