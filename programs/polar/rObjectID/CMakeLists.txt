##################################################################
# Build Object Driver programs
#
##################################################################
#
add_executable(rObj_HysDriver 
               rObj_HysDriver.cc 
               rRadialObjectIdentifier.cc
)
target_include_directories(rObj_HysDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(rObj_HysDriver PRIVATE -Wall -Wextra -Wshadow -pedantic -Wunused-parameter)
#don't show me warnings in Toomeys code
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/config/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/datatype/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/image/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/index/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/notifier/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/plugin/ )
target_include_directories(rObj_HysDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/watcher/ )

install(TARGETS rObj_HysDriver DESTINATION ${CMAKE_INSTALL_BINDIR})
target_link_libraries(rObj_HysDriver PRIVATE rapio)

#
add_executable(rObj_MultiThresholdDriver
  rObj_MultiThresholdDriver.cc
  rRadialObjectIdentifier.cc
)
target_link_libraries(rObj_MultiThresholdDriver PRIVATE rapio)
target_include_directories(rObj_MultiThresholdDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(rObj_MultiThresholdDriver PRIVATE -Wall -Wextra -Wshadow -pedantic -Wunused-parameter)
#don't show me warnings in Toomeys code
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/config/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/datatype/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/image/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/index/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/notifier/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/plugin/ )
target_include_directories(rObj_MultiThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/watcher/ )
install(TARGETS rObj_MultiThresholdDriver DESTINATION ${CMAKE_INSTALL_BINDIR})

#
add_executable(rObj_SingleThresholdDriver
  rObj_SingleThresholdDriver.cc
  rRadialObjectIdentifier.cc
)
target_link_libraries(rObj_SingleThresholdDriver PRIVATE rapio)
target_include_directories(rObj_SingleThresholdDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(rObj_SingleThresholdDriver PRIVATE -Wall -Wextra -Wshadow -pedantic -Wunused-parameter)
#don't show me warnings in Toomeys code
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/config/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/datatype/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/image/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/index/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/notifier/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/plugin/ )
target_include_directories(rObj_SingleThresholdDriver SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/base/watcher/ )
install(TARGETS rObj_SingleThresholdDriver DESTINATION ${CMAKE_INSTALL_BINDIR})
