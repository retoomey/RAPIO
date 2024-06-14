##################################################################
# CMakeLists for building programs
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

##################################################################
# Shared code
add_library(nseshared STATIC 
  nsePoint.cc
  nseProfile.cc
)

##################################################################
# NSE
#add_executable(nse
#  rNSE.cc
#)
#target_link_libraries(PRIVATE nseshared PUBLIC rapio)
#install(TARGETS rFusion1
#   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
#)

