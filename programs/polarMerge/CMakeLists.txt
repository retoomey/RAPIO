##################################################################
# Build polarMerge
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

add_executable(rPolarVMax
  rPolarVMax.cc
)
target_link_libraries(rPolarVMax rapio)
install(TARGETS rPolarVMax
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
