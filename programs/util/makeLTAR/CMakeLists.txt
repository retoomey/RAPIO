add_executable(makeLTAR makeLTAR.cc )

# These need to be set at root level or you have compile mismatches
#Flags
#target_compile_options(makeLTAR PRIVATE
#-Wall -g -fPIC -DLINUX -std=c++17 -Wextra -pedantic -Wunused-parameter
#)
# -g (never hardcode) CMAKE_BUILD_TYPE=Debug
# -fPIC (set by root)

# Enforce C++17 for this target (RAPIO is min 17 now anyway)
#target_compile_features(makeLTAR PRIVATE cxx_std_17)

# Add compile definitions properly instead of passing -D to the compiler
#target_compile_definitions(makeLTAR PRIVATE LINUX)

# Keep strict warnings local to this algorithm
#target_compile_options(makeLTAR PRIVATE
#    -Wall -Wextra -pedantic -Wunused-parameter
#)

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
