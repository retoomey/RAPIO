# ==============================================================================
# RPATH Configuration Module v1.0
# Robert Toomey August 2026
# Many of our NSSL projects we use dated builds with absolute paths. This
# utility allows controlling RPATH for various purposes.
#
# ==============================================================================
# This module controls how the Run-Time Search Path (RPATH) is embedded into 
# the project's installed binaries. This determines how the executable finds 
# its shared libraries at runtime without relying on LD_LIBRARY_PATH.
#
# Available Modes for <PROJECT>_RPATH_MODE:
#
# 1. OFF (Default CMake Behavior)
#    - Purpose: Leaves RPATH management entirely up to standard CMake defaults. 
#      Typically, this means RPATH is stripped upon installation. Binaries will 
#      rely on standard system paths (e.g., /usr/lib) or LD_LIBRARY_PATH.
#    - Use Case: Installing via system package managers (RPM, DEB) to standard 
#      system directories.
#    - Verification: `readelf -d bin/my_app | grep RUNPATH`
#      (Output will be empty)
#
# 2. RELOCATABLE ($ORIGIN)
#    - Purpose: Embeds a relative path instructing the binary to look for 
#      libraries in a sibling directory relative to where the binary is currently 
#      running.
#    - Use Case: Portable deployments. If you zip up the install directory and 
#      extract it anywhere (e.g., /tmp/test, /home/user/app), it still works.
#    - Verification: `readelf -d bin/my_app | grep RUNPATH`
#      Output: 0x000000000000001d (RUNPATH)  Library runpath: [$ORIGIN/../lib]
#
# 3. ABSOLUTE (Fixed Prefix)
#    - Purpose: Hardcodes the exact installation path (CMAKE_INSTALL_PREFIX) 
#      into the binary. It will ONLY look in that specific directory.
#    - Use Case: Strict, side-by-side production deployments (e.g., /opt/rdm_v1 
#      and /opt/rdm_v2) where cross-contamination of libraries must be prevented.
#    - Verification: `readelf -d bin/my_app | grep RUNPATH`
#      Output: 0x000000000000001d (RUNPATH)  Library runpath: [/opt/rdm/lib]
# ==============================================================================
macro(configure_custom_rpath PROJECT_PREFIX)
    
    # Ensure GNUInstallDirs is loaded locally so lib vs lib64 is handled correctly per OS
    include(GNUInstallDirs)

    # 1. Define a multi-choice cache variable. 
    if(NOT DEFINED ${PROJECT_PREFIX}_RPATH_MODE)
        set(${PROJECT_PREFIX}_RPATH_MODE "OFF" CACHE STRING 
            "RPATH configuration mode: OFF (do nothing), RELOCATABLE ($ORIGIN), or ABSOLUTE (fixed path)")
    else()
        set(${PROJECT_PREFIX}_RPATH_MODE "${${PROJECT_PREFIX}_RPATH_MODE}" CACHE STRING 
            "RPATH configuration mode: OFF (do nothing), RELOCATABLE ($ORIGIN), or ABSOLUTE (fixed path)")
    endif()
    
    # 2. Constrain the choices for cmake-gui and ccmake
    set_property(CACHE ${PROJECT_PREFIX}_RPATH_MODE PROPERTY STRINGS "OFF" "RELOCATABLE" "ABSOLUTE")

    # 3. Establish the baseline install library variable using GNUInstallDirs (lib or lib64)
    set(${PROJECT_PREFIX}_INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

    # 4. Apply the logic based on the mode
    if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
        message(STATUS "RPATH Config: Installing to /usr. Forcing OFF mode.")
        set(${PROJECT_PREFIX}_INSTALL_LIB "/usr/${CMAKE_INSTALL_LIBDIR}")
        
    elseif(${PROJECT_PREFIX}_RPATH_MODE STREQUAL "RELOCATABLE")
        message(STATUS "RPATH Config: Setting RELOCATABLE RPATH ($ORIGIN).")
        
        if(APPLE)
            set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
        else()
            set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
        endif()
        
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    elseif(${PROJECT_PREFIX}_RPATH_MODE STREQUAL "ABSOLUTE")
        message(STATUS "RPATH Config: Setting ABSOLUTE RPATH (${CMAKE_INSTALL_LIBDIR}) for strict build isolation.")
        
        cmake_policy(SET CMP0060 NEW)
        # Dynamically evaluates to .../lib or .../lib64 depending on the system architecture
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
        set(CUSTOM_THIRDDIR "${CMAKE_INSTALL_PREFIX}")

    else()
        message(STATUS "RPATH Config: OFF mode. Leaving CMake to its standard handling.")
    endif()
endmacro()
