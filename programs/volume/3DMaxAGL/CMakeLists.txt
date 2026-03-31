##################################################################
# CMakeLists for building programs
#
##################################################################
# MaxAGL
# Since this alg will work on a 3D grid from merger/fusion,
# it should be modular like 3DVIL.  
# FIXME: Clean up this API, CMake, etc.
#
add_library(rapioalgMaxAGL SHARED
  rMaxAGL.cc 
)
target_link_libraries(rapioalgMaxAGL PRIVATE rapio_interface)

install(TARGETS rapioalgMaxAGL
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(rMaxAGL
  rMaxAGL_main.cc
)
target_link_libraries(rMaxAGL PRIVATE rapio rapioalgMaxAGL)
install(TARGETS rMaxAGL
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Build tests conditionally
if(BUILD_TESTS)
  add_executable(rTestMaxAGL
    rTestMaxAGL.cc
  )
  target_link_libraries(rTestMaxAGL PRIVATE rapio rapioalgMaxAGL)
  
  # Register the test with CTest
  add_test(NAME rTestMaxAGL COMMAND rTestMaxAGL)
  
  # Optional: Keep the install if you actually distribute the test binaries
  # install(TARGETS rTestMaxAGL DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
