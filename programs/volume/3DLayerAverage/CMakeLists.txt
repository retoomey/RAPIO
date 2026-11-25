##################################################################
# CMakeLists for building 3DLayerAverage
#
##################################################################
add_library(rapioalgLayerAverage SHARED
  r3DLayerAverage.cc
)
target_link_libraries(rapioalgLayerAverage PRIVATE rapio_interface)

install(TARGETS rapioalgLayerAverage
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(r3DLayerAverage
  r3DLayerAverage_main.cc
)
target_link_libraries(r3DLayerAverage PRIVATE rapio rapioalgLayerAverage)
install(TARGETS r3DLayerAverage
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  add_executable(rTest3DLayerAverage
    rTest3DLayerAverage.cc
  )
  target_link_libraries(rTest3DLayerAverage PRIVATE rapio rapioalgLayerAverage)

  # Register the test with CTest
  addRAPIOTest(rTest3DLayerAverage COMMAND rTest3DLayerAverage)
endif()
