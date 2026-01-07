##################################################################
# CMakeLists for building programs
#
##################################################################
# 3DVil
# This is my first experiment at 'dual' calling an algorithm,
# either from command line or another algorithm.
# We'll try to make dynamic modules of algorithms, to allow
# a program like fusionAlgs to call them as a group.
#
# FIXME: This creating a loading alg 'module' could be a function
add_library(rapioalgVIL SHARED
  r3DVil.cc 
)
target_link_libraries(rapioalgVIL PRIVATE rapio_interface)

install(TARGETS rapioalgVIL
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(r3DVil
  r3DVil_main.cc
)
target_link_libraries(r3DVil PRIVATE rapio rapioalgVIL)
install(TARGETS r3DVil
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)
