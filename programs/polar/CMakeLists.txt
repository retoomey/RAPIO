##################################################################
# Build polarMerge
#
add_executable(rPolarVMax
  rPolarVMax.cc
)
target_link_libraries(rPolarVMax PRIVATE rapio)
target_include_directories(rPolarVMax PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rPolarVMax
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rEchoTop
  rEchoTop.cc
)
target_link_libraries(rEchoTop PRIVATE rapio)
target_include_directories(rEchoTop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rEchoTop
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)
