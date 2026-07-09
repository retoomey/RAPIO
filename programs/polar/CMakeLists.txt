##################################################################
# Build polarMerge
#
#Apply strict warnings ONLY to your code
#target_compile_options(rLLSDPolar PRIVATE -Wall -Wextra)
#target_include_directories(rLLSDPolar SYSTEM PRIVATE /home/john.krause/RAPIO/base )

add_executable(rPolarVMax
  rPolarVMax.cc
)
target_link_libraries(rPolarVMax PRIVATE rapio)
target_include_directories(rPolarVMax PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rPolarVMax
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rEchoTop
  rEchoTop.cc
)
target_link_libraries(rEchoTop PRIVATE rapio)
target_include_directories(rEchoTop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rEchoTop
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rLLSDPolar
  rLLSDPolar.cc
)
target_link_libraries(rLLSDPolar rapio)
target_include_directories(rLLSDPolar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rLLSDPolar
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rPolarVIL
  rPolarVIL.cc
)
target_link_libraries(rPolarVIL rapio)
target_include_directories(rPolarVIL PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rPolarVIL
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# 3. Register Subdirectories
add_subdirectory(rPreProAI)
add_subdirectory(rDRradial)
add_subdirectory(rPreProQC)
