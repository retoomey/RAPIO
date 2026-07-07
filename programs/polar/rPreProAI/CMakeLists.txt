#
add_executable(rPreProAI 
                 corr_attenuation.cc 
                 fastMedian.cc 
                 phase_computations.cc 
                 calc_kdp.cc 
                 computeDR.cc 
                 rPreProAI.cc)
#Add both the local includes and all the rapio includes 
target_include_directories(rPreProAI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Apply strict warnings ONLY to your code
target_compile_options(rPreProAI PRIVATE -Wall -Wextra -Wshadow -pedantic -Wunused-parameter)
#don't show me warnings in Toomeys code
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/datatype/ )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/image/ )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/index/ )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/notifier/ )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/plugin/ )
target_include_directories(rPreProAI SYSTEM PRIVATE /home/john.krause/RAPIO/base/watcher/ )
#add links
target_link_libraries(rPreProAI PRIVATE rapio)

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS rPreProAI
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

