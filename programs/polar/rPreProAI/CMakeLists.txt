#
add_executable(rpreproai 
                 corr_attenuation.cc 
                 fastMedian.cc 
                 phase_computations.cc 
                 calc_kdp.cc 
                 computeDR.cc 
                 rPreProAI.cc)
#Add both the local includes and all the rapio includes 
target_include_directories(rpreproai PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#add links
target_link_libraries(rpreproai PRIVATE rapio)

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS rpreproai
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

