find_package(GDAL)

if (GDAL_FOUND)

  # Define files for the library
  addRAPIOModule(rapiogdal SHARED
    rGDALDataTypeImp.cc
    rIOGDAL.cc
  )

  # Link to gdal headers
  target_include_directories(rapiogdal PRIVATE ${GDAL_INCLUDE_DIR})

  # Link to gdal libraries
  target_link_libraries(rapiogdal PRIVATE ${GDAL_LIBRARIES})
  
  addRAPIONote("      Found: GDAL")
else()
  addRAPIONote("      Missing: GDAL (Won't be able to write geoTiff)")
endif(GDAL_FOUND)
