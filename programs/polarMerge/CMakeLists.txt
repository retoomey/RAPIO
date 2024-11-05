##################################################################
# Build polarMerge
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

add_executable(rPolarMerge
  rPolarMerge.cc
)
target_link_libraries(rPolarMerge rapio)
install(TARGETS rPolarMerge
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
