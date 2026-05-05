# Define files for the module
addRAPIOModule(rapiohmrg SHARED
  rIOHmrg.cc
  rHmrgProductInfo.cc
  rHmrgRadialSet.cc
  rHmrgLatLonGrids.cc
)
# Explicitly link zlib since this module calls gzopen/gzread directly
target_link_libraries(rapiohmrg PRIVATE ${ZLIB_LIBRARIES})
addRAPIONote("      Found: hmrg")
