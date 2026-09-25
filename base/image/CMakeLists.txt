#Image worth build unique since it could get large
add_library(rapio_image OBJECT
  rNearestNeighbor.cc
  rCressman.cc
  rBilinear.cc
  rThresholdFilter.cc
  rPercentFilter.cc
  rDilateFilter.cc
  rDespeckleFilter.cc
)
# We're only to link to rapio shared, this is a logical grouping
set_target_properties(rapio_image PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Inherit all the include directories and definitions from the interface
target_link_libraries(rapio_image PRIVATE rapio_interface)
