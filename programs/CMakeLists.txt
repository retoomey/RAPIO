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
# Algorithms
#
add_subdirectory(fusion)
add_subdirectory(nse)

##################################################################
# Modular Algorithms (callable from groups such as FusionAlgs)
#
add_subdirectory(3DVIL)

##################################################################
# Utility/GUI stuff
#
add_subdirectory(util)
add_subdirectory(RESTserver)
