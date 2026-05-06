#
add_executable(rDRradial computeDR.cc rDRradial.cc)
#Add both the local includes and all the rapio includes 
target_include_directories(rDRradial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#add links
target_link_libraries(rDRradial PRIVATE rapio)

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS rDRradial
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

