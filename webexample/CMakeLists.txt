cmake_minimum_required(VERSION 3.14)
project(ImGuiWithEmscripten)

include(ExternalProject)

# Root for building in the current build folder
set(ROOT_DIRECTORY "${CMAKE_BINARY_DIR}/external")
set(IMGUI_SOURCE_DIR ${ROOT_DIRECTORY}/imgui)
set(EMSCRIPTEN_SOURCE_DIR ${ROOT_DIRECTORY}/emscripten)

##################################################################
# We clone in configure stage to ensure the .cpp files we will build
# in our target exist during the rest of the configure dependency checks.
#

# Ensure ROOT_DIRECTORY exists
# This also helps CMake understand the git clone of IMGUI is
# done during configuration stage.
file(MAKE_DIRECTORY ${ROOT_DIRECTORY})

# Clone ImGui repository if it doesn't exist
if(NOT EXISTS ${IMGUI_SOURCE_DIR})
    message(STATUS "Cloning ImGui repository...")
    execute_process(
        COMMAND git clone https://github.com/ocornut/imgui.git ${IMGUI_SOURCE_DIR}
        WORKING_DIRECTORY ${ROOT_DIRECTORY}
    )
else()
    message(STATUS "ImGui repository already cloned.")
endif()

# Define paths to the ImGui files
set(IMGUI_FILES 
    ${IMGUI_SOURCE_DIR}/imgui.cpp 
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp 
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp 
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

##################################################################
# Set up Emscripten as an external project
#
ExternalProject_Add(
    emscripten
    PREFIX ${EMSCRIPTEN_SOURCE_DIR}
    GIT_REPOSITORY https://github.com/emscripten-core/emsdk.git
    GIT_TAG main
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ./emsdk install latest && ./emsdk activate latest
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)

# Ensure Emscripten environment is sourced before building
add_custom_target(emscripten_env
    COMMAND ${CMAKE_COMMAND} -E echo "Sourcing Emscripten environment"
    PREFIX ${EMSCRIPTEN_SOURCE_DIR}
    COMMAND /bin/bash -c "source ${EMSCRIPTEN_SOURCE_DIR}/src/emscripten/emsdk_env.sh"
)

add_dependencies(emscripten_env emscripten)

# Add the executable
add_executable(webbuild main.cpp ${IMGUI_FILES})

# Link ImGui and Emscripten
add_dependencies(webbuild emscripten emscripten_env)

target_include_directories(webbuild PRIVATE ${IMGUI_SOURCE_DIR})
target_include_directories(webbuild PRIVATE ${IMGUI_SOURCE_DIR}/backends)

# Set the Emscripten specific compiler and linker flags
set_target_properties(webbuild PROPERTIES
   COMPILE_FLAGS "-sUSE_SDL=2"
   LINK_FLAGS "-lembind --shell-file ${CMAKE_SOURCE_DIR}/template.html -sUSE_SDL=2"
   OUTPUT_NAME "index.html"
)

# Ensure that emscripten is used as the compiler
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_SOURCE_DIR}/src/emscripten/upstream/emscripten/em++")

# Install rules to place the generated files into the desired location
# Generated files are BINARY, non-generated from SOURCE
install(FILES
    ${CMAKE_BINARY_DIR}/index.html
    ${CMAKE_BINARY_DIR}/index.js
    ${CMAKE_BINARY_DIR}/index.wasm
    ${CMAKE_SOURCE_DIR}/styles.css
    ${CMAKE_SOURCE_DIR}/favicon.ico
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/web
)

