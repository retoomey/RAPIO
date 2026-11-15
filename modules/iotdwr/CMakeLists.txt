addRAPIOModule(rapiotdwr SHARED
  rIOTDWR.cc
  rTDWRDecoder.cc
  rTDWRAccumulator.cc
  rTDWRStreamReader.cc
)

addRAPIONote("      Found: TDWR (Terminal Doppler Weather Radar)")

# Standalone CLI application for TDWR streaming ingest
add_executable(rTDWRStream
  rTDWRStream.cc
)

target_include_directories(rTDWRStream PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rTDWRStream PRIVATE
  rapio
  rapiotdwr
)

install(TARGETS rTDWRStream
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  if(NOT TARGET Boost::unit_test_framework)
    find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
  endif()

  add_executable(rTestTDWR
    rTestTDWR.cc
  )

  target_include_directories(rTestTDWR PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests
  )

  target_link_libraries(rTestTDWR PRIVATE
    Boost::unit_test_framework
    rapio
    rapiotdwr
  )

  addRAPIOTest(rTestTDWR
    COMMAND rTestTDWR --log_level=test_suite
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()
