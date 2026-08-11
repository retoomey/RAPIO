add_executable(makeLTAR makeLTAR.cc )
#Add both the local includes and all the rapio includes 
target_include_directories(makeLTAR PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(makeLTAR PRIVATE "$ENV{HOME}/include/rapio")
#add links
target_link_libraries(makeLTAR PRIVATE rapio)

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS makeLTAR 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install Python scripts with executable permissions to the bin directory
install(PROGRAMS 
        makeLTAR_driver.py
        drive_getRadarData.py
        getRadarData.py
        cleanUpFiles.py
        DESTINATION ${CMAKE_INSTALL_BINDIR})
