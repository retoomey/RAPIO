# The simple algorithm example
add_executable(rsimplealg rSimpleAlg.cc)
target_link_libraries(rsimplealg PRIVATE rapio)
target_include_directories(rsimplealg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The simple tool/executable example
add_executable(rsimpleprogram rSimpleProgram.cc)
target_link_libraries(rsimpleprogram PRIVATE rapio)
target_include_directories(rsimpleprogram PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The grib example
add_executable(rgribexample rGribExample.cc)
target_link_libraries(rgribexample PRIVATE rapio)
target_include_directories(rgribexample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Standard CMake practice uses RUNTIME DESTINATION bin
install(TARGETS rsimplealg rsimpleprogram rgribexample
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
