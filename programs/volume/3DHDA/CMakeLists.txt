##################################################################
# CMakeLists for building 3DHDA
#
##################################################################
add_library(rapioalgHDA SHARED
  r3DHDA.cc
)
target_link_libraries(rapioalgHDA PRIVATE rapio_interface)

install(TARGETS rapioalgHDA
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(r3DHDA
  r3DHDA_main.cc
)
target_link_libraries(r3DHDA PRIVATE rapio rapioalgHDA)
install(TARGETS r3DHDA
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  add_executable(rTest3DHDA
    rTest3DHDA.cc
  )
  target_link_libraries(rTest3DHDA PRIVATE rapio rapioalgHDA)

  # Register the test with CTest
  addRAPIOTest(rTest3DHDA COMMAND rTest3DHDA)
endif()
