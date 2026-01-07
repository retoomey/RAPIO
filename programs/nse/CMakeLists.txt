##################################################################
# CMakeLists for building programs
#

##################################################################
# Shared code
add_library(nseshared STATIC 
  nsePoint.cc
  nseProfile.cc
)

target_include_directories(nseshared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nseshared PUBLIC rapio)
##################################################################
# NSE
add_executable(rNSE
  rNSE.cc
)
target_include_directories(rNSE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rNSE PRIVATE nseshared)

install(TARGETS rNSE
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

