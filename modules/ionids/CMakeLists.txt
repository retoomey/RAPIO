addRAPIOModule(rapionids SHARED
  rIONIDS.cc
  rNIDSRadialSet.cc
  rNIDSHailIndexTable.cc
  rNIDSMesoDetectionTable.cc
  rNIDSMesoTable.cc
  rNIDSSTITable.cc
  rNIDSStormCellTable.cc
  rNIDSStormStructureTable.cc
  rNIDSTVSTable.cc
  rNIDSUtil.cc
  # NIDS product table (required)
  rConfigNIDSInfo.cc

  # Blocks of Level III
  rBlockMessageHeader.cc
  rBlockProductDesc.cc
  rBlockProductSymbology.cc
  rBlockRadialSet.cc
)

addRAPIONote("      Found: NIDS Level III")

# Build tests conditionally
if(RAPIO_BUILD_TESTS)
  if(NOT TARGET Boost::unit_test_framework)
    find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
  endif()

  add_executable(rTestNIDS
    rTestNIDS.cc
  )

  target_include_directories(rTestNIDS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests
  )

  target_link_libraries(rTestNIDS PRIVATE
    Boost::unit_test_framework
    rapio
    rapionids
  )

  addRAPIOTest(rTestNIDS
    COMMAND rTestNIDS --log_level=test_suite
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()
