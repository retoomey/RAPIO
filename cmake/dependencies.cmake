# cmake/dependencies.cmake
# ---------------------------------------------------------
# Centralized Git Tags / Commits / Branches for all external dependencies
#
# Toomey May 2026
#   Putting all the GIT version tags (and other critical
#   version things in one place.)
#   We don't want 'complete' bleeding edge because things can
#   suddenly break.  But occasionally we should move the tags
#   forward and test/update.   
# ---------------------------------------------------------

# It's better to use named tags if possible, otherwise can get
# reference failures with hashes (git needs the history)
# Using a tag name allows Git to resolve a specific, 
# named entry point to fetch only that single slice of history, 
# whereas a hash is an anonymous reference that Git cannot 
# "find" without first downloading the broader history to locate it.

# Web GUI
set(IMGUI_GIT_TAG      "v1.90.4")
set(EMSCRIPTEN_GIT_TAG "3d6d8ee") # Release 3.1.74 Dec 13, 2024

# WGRIB2 / G2C (need to match versions pretty close here)
#set(G2C_GIT_TAG        "cc3e5b6") # Tag for the version 1.8.0 release Oct 27, 2023
#set(WGRIB2_GIT_TAG     "v3.8.0")  # Started breaking. Hoping no roll back needed
#set(G2C_GIT_TAG        "404f1dd") # Jan 12, 2026 Use GIT_SHALLOW 0
#set(WGRIB2_GIT_TAG     "d1cef8f")  # Feb 27, 2026 Use GIT_SHALLOW 0

set(G2C_GIT_TAG        "v2.3.0") # Nov 4, 2025
set(WGRIB2_GIT_TAG     "v3.8.0") # Nov 6, 2025

# Logging & Formatting
set(FMT_GIT_TAG        "12.1.0")
set(SPDLOG_GIT_TAG     "v1.16.0")
