##################################################################
# CMakeLists for building 3DColumnStats
#
##################################################################
add_library(rapioalgColumnStats SHARED
  r3DColumnStats.cc
)
target_link_libraries(rapioalgColumnStats PRIVATE rapio_interface)

install(TARGETS rapioalgColumnStats
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(r3DColumnStats
  r3DColumnStats_main.cc
)
target_link_libraries(r3DColumnStats PRIVATE rapio rapioalgColumnStats)
install(TARGETS r3DColumnStats
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  add_executable(rTest3DColumnStats
    rTest3DColumnStats.cc
  )
  target_link_libraries(rTest3DColumnStats PRIVATE rapio rapioalgColumnStats)

  # Register the test with CTest
  addRAPIOTest(rTest3DColumnStats COMMAND rTest3DColumnStats)
endif()
