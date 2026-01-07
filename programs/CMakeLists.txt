##################################################################
# CMakeLists for building programs
#

##################################################################
# Grib2Reader 
add_executable(rGrib2Reader
	rGrib2Reader.cc
)
target_link_libraries(rGrib2Reader PRIVATE rapio)
target_include_directories(rGrib2Reader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rGrib2Reader
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Make the rWebGUI here, since the RESTserver builds with em++
# and we'll build with normal c++ compiler
if (BUILD_WEB_MODULE)
  add_executable(rWebGUI
    RESTserver/rWebGUI.cc
  )
  target_link_libraries(rWebGUI PRIVATE rapio)
  install(TARGETS rWebGUI
     DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  add_subdirectory(RESTserver)
endif (BUILD_WEB_MODULE)

##################################################################
# Algorithms
#
add_subdirectory(fusion)
add_subdirectory(nse)
add_subdirectory(polar)

##################################################################
# Modular Algorithms (callable from groups such as FusionAlgs)
#
add_subdirectory(3DVIL)

##################################################################
# Utility/GUI stuff
#
add_subdirectory(util)
