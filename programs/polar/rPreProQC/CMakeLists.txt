#
add_executable(rPreProQC
               computeFilter.cc
               computeDRmask.cc
               computeLTARmask.cc
               computeStdPhiDPmask.cc
               rPreProQC.cc)
#Add both the local includes and all the rapio includes 
target_include_directories(rPreProQC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Apply strict warnings ONLY to your code
target_compile_options(rPreProQC PRIVATE -Wall -Wextra -Wshadow -pedantic -Wunused-parameter)
#don't show me warnings in RAPIO code
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/config/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/datatype/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/image/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/index/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/notifier/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/plugin/ )
target_include_directories(rPreProQC SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/watcher/ )
#add links
target_link_libraries(rPreProQC PRIVATE rapio)

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS rPreProQC
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

