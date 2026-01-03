# We wants headers in the current directory, even if bracketed.
include_directories(.)

# Include all headers RAPIO uses and RAPIO headers
include_directories(${RAPIO_INCLUDE_DIRS})

# Normally use ExternalProject_Add, but it seems the
# recommendation for private libraries is FetchContent
include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.16.0  # Use a specific stable tag!
)

FetchContent_MakeAvailable(spdlog)
# Force the spdlog target to be compiled with -fPIC
set_target_properties(spdlog PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
)

addRAPIONote("      Found: SPDLogger")

# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

add_library(rapiologspd SHARED
  rLogSPD.cc
)

target_link_libraries(rapiologspd
  spdlog
  ${RAPIO_INCLUDE_LIBRARIES}
)

# Install the module
install(TARGETS rapiologspd
   LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
