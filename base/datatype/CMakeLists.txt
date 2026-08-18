add_library(rapio_datatype OBJECT
  rAttributeDataType.cc 
  rBinaryTable.cc 
  rDataGrid.cc
  rDataTable.cc
  rDataType.cc
  rFusionBinaryTable.cc
  rGDALDataType.cc
  rGribDataType.cc
  rImageDataType.cc
  rLatLonArea.cc
  rLatLonGrid.cc  
  rLatLonGridIterator.cc 
  rLatLonGridProjection.cc
  rLatLonHeightGrid.cc 
  rLatLonHeightGridIterator.cc 
  rLatLonHeightGridProjection.cc 
  rLLHGridN2D.cc
  rMergerBinaryTable.cc
  rMultiDataType.cc
  rNetcdfDataType.cc
  rPTreeData.cc
  rRadialSet.cc
  rRadialSetIterator.cc
  rRadialSetProjection.cc
  rWindBinaryTable.cc
)
# We're only to link to rapio shared, this is a logical grouping
set_target_properties(rapio_datatype PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Inherit all the include directories and definitions from the interface
target_link_libraries(rapio_datatype PRIVATE rapio_interface)
