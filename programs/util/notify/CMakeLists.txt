include(FetchContent)

# Fetch the RDM repository
FetchContent_Declare(
    rdm
    # SOURCE_DIR mylocalrdmgit  // if you're changing both libraries at once
    GIT_REPOSITORY https://github.com/retoomey/rdm.git
    GIT_TAG        main # Or point to a specific commit hash for stability
)

# Disable RDM's internal tests to speed up the RAPIO build
set(RDM_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Make RDM targets available (rdmcore, rdmnetsunrpc, rdmstorepq)
FetchContent_MakeAvailable(rdm)

# Build the RAPIO wrapper algorithm
add_executable(rNotify
    rNotify.cc
)

# Link against RAPIO and the RDM core library
target_link_libraries(rNotify PRIVATE rapio rdmcore spdlog::spdlog)
target_include_directories(rNotify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Ensure we can find the RDM core headers
# FetchContent populates rdm_SOURCE_DIR
target_include_directories(rNotify PRIVATE ${rdm_SOURCE_DIR}/core)

install(TARGETS rNotify
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
