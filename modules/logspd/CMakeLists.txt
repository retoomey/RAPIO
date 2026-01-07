include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.16.0
)

FetchContent_MakeAvailable(spdlog)

# Force spdlog to be PIC (shared library friendly)
set_target_properties(spdlog PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(rapiologspd SHARED
  rLogSPD.cc
)

# Target libraries use the rapio header interface
target_link_libraries(rapiologspd
  PRIVATE rapio_interface
  PRIVATE spdlog 
)

# Target headers, add this source directory
target_include_directories(rapiologspd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Check all symbols resolved.  We try to link to as little as possible
# with dynamic modules to reduce size, compile time and duplication.
# This option allows making sure module is valid in build
if(RAPIO_STRICT_SYMBOL_CHECK)
  target_link_options(rapiologspd PRIVATE "-Wl,--no-undefined")
  target_link_libraries(rapiologspd PRIVATE rapio)
endif()

# Install the module
include(GNUInstallDirs)
install(TARGETS rapiologspd
    EXPORT rapioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

addRAPIONote("      Found: SPDLogger")

