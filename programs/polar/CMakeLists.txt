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

add_executable(rEchoTop
  rEchoTop.cc
)
target_link_libraries(rEchoTop rapio)
install(TARGETS rEchoTop
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
