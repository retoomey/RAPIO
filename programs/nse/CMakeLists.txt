##################################################################
# CMakeLists for building programs
#
project(rNse)

link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

##################################################################
# Shared code
add_library(nseshared STATIC 
  nsePoint.cc
  nseProfile.cc
)

target_link_libraries(nseshared PUBLIC rapio)
##################################################################
# NSE
add_executable(rNSE
  rNSE.cc
)
target_link_libraries(rNSE PRIVATE nseshared)

install(TARGETS rNSE
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

