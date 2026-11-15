addRAPIOModule(rapionexrad SHARED
  rIONEXRAD.cc
  rNEXRADDecoder.cc
  rNEXRADAccumulator.cc
  rNEXRADFileReader.cc
  rNEXRADStreamReader.cc
)

# Alias for backward compatibility
add_library(rapioar2v ALIAS rapionexrad)

addRAPIONote("      Found: NEXRAD Level II (.ar2v)")

# Standalone CLI application for LDM streaming ingest (ldm2netcdf replacement)
add_executable(rL2Stream
  rL2Stream.cc
)

target_include_directories(rL2Stream PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/modules/iotdwr
)

target_link_libraries(rL2Stream PRIVATE
  rapio
  rapionexrad
  rapiotdwr
)

install(TARGETS rL2Stream
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  if(NOT TARGET Boost::unit_test_framework)
    find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
  endif()

  add_executable(rTestNEXRAD
    rTestNEXRAD.cc
  )

  target_include_directories(rTestNEXRAD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests
  )

  target_compile_definitions(rTestNEXRAD PRIVATE
    NEXRAD_TEST_FILE="${CMAKE_CURRENT_SOURCE_DIR}/KATX20261110_065712_V06.ar2v"
    AR2V_TEST_FILE="${CMAKE_CURRENT_SOURCE_DIR}/KATX20261110_065712_V06.ar2v"
  )

  target_link_libraries(rTestNEXRAD PRIVATE
    Boost::unit_test_framework
    rapio
    rapionexrad
  )

  addRAPIOTest(rTestNEXRAD
    COMMAND rTestNEXRAD --log_level=test_suite
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  # Provide rTestAR2V alias target for backward compatibility
  add_custom_target(rTestAR2V ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rTestNEXRAD> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rTestAR2V
    DEPENDS rTestNEXRAD
  )
endif()
