##################################################################
# CMakeLists for building 3DSCIT
#
##################################################################
add_library(rapioalgSCIT SHARED
  r3DSCIT.cc
)
target_link_libraries(rapioalgSCIT PRIVATE rapio_interface)

install(TARGETS rapioalgSCIT
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(r3DSCIT
  r3DSCIT_main.cc
)
target_link_libraries(r3DSCIT PRIVATE rapio rapioalgSCIT)
install(TARGETS r3DSCIT
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  add_executable(rTest3DSCIT
    rTest3DSCIT.cc
  )
  target_link_libraries(rTest3DSCIT PRIVATE rapio rapioalgSCIT)

  # Register the test with CTest
  addRAPIOTest(rTest3DSCIT COMMAND rTest3DSCIT)
endif()
