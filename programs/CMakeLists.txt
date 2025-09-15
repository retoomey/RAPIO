##################################################################
# CMakeLists for building programs
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

# Need this to add the BOOST setting or the rError.cc will complain about v2
# FIXME: not sure needed anymore
add_definitions(-DBOOST_LOG_DYN_LINK)

##################################################################
# Grib2Reader 
add_executable(rGrib2Reader
	rGrib2Reader.cc
)
target_link_libraries(rGrib2Reader rapio)
install(TARGETS rGrib2Reader
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Make the rWebGUI here, since the RESTserver builds with em++
# and we'll build with normal c++ compiler
if (BUILD_WEB_MODULE)
  add_executable(rWebGUI
    RESTserver/rWebGUI.cc
  )
  target_link_libraries(rWebGUI rapio)
  install(TARGETS rWebGUI
     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
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
