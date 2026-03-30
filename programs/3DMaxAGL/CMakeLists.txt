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
# For the moment build the test always.
# FIXME: update our code to have a test flag and ctest work, etc.
# Going to fix that in a separate check-in
add_executable(rTestMaxAGL
  rTestMaxAGL.cc
)
target_link_libraries(rTestMaxAGL PRIVATE rapio rapioalgMaxAGL)
install(TARGETS rTestMaxAGL
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

