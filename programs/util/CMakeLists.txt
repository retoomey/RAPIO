##################################################################
# Build utilities
#
add_executable(rdump
  rDump.cc
)
target_link_libraries(rdump rapio)
target_include_directories(rdump PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rdump
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)
add_executable(rcopy
  rCopy.cc
)
target_link_libraries(rcopy rapio)
target_include_directories(rcopy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rcopy
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)
add_executable(rwatch
  rWatch.cc
)
target_link_libraries(rwatch rapio)
target_include_directories(rwatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rwatch
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rRemap
  rRemap.cc
)
target_link_libraries(rRemap rapio)
target_include_directories(rRemap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rRemap
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(rMakeIndex
  rMakeIndex.cc
)
target_link_libraries(rMakeIndex rapio)
target_include_directories(rMakeIndex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rMakeIndex
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# MakeFakeRadarData 
# FIXME: Not sure if we're keeping this. I'm experimenting with
# a fakeIndex generator API which may be better
add_executable(rMakeFakeRadarData
  rMakeFakeRadarData.cc
)
target_link_libraries(rMakeFakeRadarData rapio)
target_include_directories(rMakeFakeRadarData PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rMakeFakeRadarData
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Single tile generation 
#
add_executable(rGetTile
  rGetTile.cc
)
target_link_libraries(rGetTile rapio)
target_include_directories(rGetTile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rGetTile
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Legend generator (SVG)
#
add_executable(rGetLegend
  rGetLegend.cc
)
target_link_libraries(rGetLegend rapio)
target_include_directories(rGetLegend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rGetLegend
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

##################################################################
# Colormap generator (JSON)
#
add_executable(rGetColorMap
  rGetColorMap.cc
)
target_link_libraries(rGetColorMap rapio)
target_include_directories(rGetColorMap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS rGetColorMap
   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the makeindex script
install_python("makeindex.py" ${CMAKE_CURRENT_SOURCE_DIR})

