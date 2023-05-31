# Toomey Sept 2021
# RAPIO base cmake file
#

# Set variable of all headers we use
# We 'could' file(GLOB HEADER_FILES "*.h") but
# this means when we add a file we have to force cmake
# to recognize the change so for now we'll hard list the files
# ls -1 *.h
set(HEADER_FILES
${CMAKE_CURRENT_BINARY_DIR}/RAPIO.h
${CMAKE_CURRENT_BINARY_DIR}/config.h
ccronexpr.h
rAlgConfigFile.h
rAlgorithm.h
rArith.h
rArray.h
rAWS.h
rBimap.h
rBinaryIO.h
rBinaryTable.h
rBitset.h
rColor.h
rColorMap.h
rColorTerm.h
rConfigDirectoryMapping.h
rConfig.h
rConfigIODataType.h
rConfigLogging.h
rConfigParamGroup.h
rConfigRadarInfo.h
rConstants.h
rCurlConnection.h
rDataFilter.h
rDataGrid.h
rData.h
rDataProjection.h
rDataStore2D.h
rDataStore3D.h
rDataStore.h
rDataTable.h
rDataType.h
rDataTypeHistory.h
rDirWatcher.h
rElevationVolume.h
rError.h
rEvent.h
rEventLoop.h
rEventTimer.h
rEXERecordNotifier.h
rEXEWatcher.h
rFactory.h
rFAMWatcher.h
rFileIndex.h
rFMLIndex.h
rFMLRecordNotifier.h
rGDALDataType.h
rGribDataType.h
rHeartbeat.h
rIJK.h
rImageDataType.h
rIndexType.h
rIODataType.h
rIOFile.h
rIO.h
rIOIndex.h
rIOJSON.h
rIOURL.h
rIOWatcher.h
rIOXML.h
rLatLonGrid.h
rLatLonHeightGrid.h
rLength.h
rLLCoverageArea.h
rLL.h
rLLH.h
rNamedAny.h
rNetcdfDataType.h
rOption.h
rOptionList.h
rOS.h
rProcessTimer.h
rProject.h
rPTreeData.h
rRadialSet.h
rRadialSetLookup.h
rRAPIOAlgorithm.h
rRAPIOData.h
rRapio.h
rRAPIOOptions.h
rRAPIOProgram.h
rRecordFilter.h
rRecord.h
rRecordNotifier.h
rRecordQueue.h
rSecure.h
rSentinelDouble.h
rSignals.h
rSparseVector.h
rStaticMethodFactory.h
rStreamIndex.h
rStrings.h
rTerrainBlockage2me.h
rTerrainBlockage.h
rTerrainBlockageLak.h
rTimeDuration.h
rTime.h
rUnit.h
rURL.h
rUtility.h
rValueCompressor.h
rVolumeValueResolver.h
rWebIndex.h
rWebIndexWatcher.h
rWebServer.h
rXMLIndex.h
rXYZ.h
)

# Set variable of all sources we use
# ls -1 *.c *.cc
set(SOURCE_FILES
ccronexpr.c
rAlgConfigFile.cc
rArray.cc
rAWS.cc
rBimap.cc
rBinaryIO.cc
rBinaryTable.cc
rBitset.cc
rColor.cc
rColorMap.cc
rColorTerm.cc
rConfig.cc
rConfigDirectoryMapping.cc
rConfigIODataType.cc
rConfigLogging.cc
rConfigParamGroup.cc
rConfigRadarInfo.cc
rConstants.cc
rCurlConnection.cc
rDataFilter.cc
rDataGrid.cc
rDataProjection.cc
rDataTable.cc
rDataType.cc
rDataTypeHistory.cc
rDirWatcher.cc
rElevationVolume.cc
rError.cc
rEventLoop.cc
rEventTimer.cc
rEXERecordNotifier.cc
rEXEWatcher.cc
rFAMWatcher.cc
rFileIndex.cc
rFMLIndex.cc
rFMLRecordNotifier.cc
rGDALDataType.cc
rGribDataType.cc
rHeartbeat.cc
rIJK.cc
rImageDataType.cc
rIndexType.cc
rIODataType.cc
rIOFile.cc
rIOIndex.cc
rIOJSON.cc
rIOURL.cc
rIOWatcher.cc
rIOXML.cc
rLatLonGrid.cc
rLatLonHeightGrid.cc
rLength.cc
rLL.cc
rLLCoverageArea.cc
rLLH.cc
rNetcdfDataType.cc
rOption.cc
rOptionList.cc
rOS.cc
rProcessTimer.cc
rProject.cc
rPTreeData.cc
rRadialSet.cc
rRadialSetLookup.cc
rRAPIOAlgorithm.cc
rRAPIOData.cc
rRAPIOOptions.cc
rRAPIOProgram.cc
rRecord.cc
rRecordFilter.cc
rRecordNotifier.cc
rRecordQueue.cc
rSecure.cc
rSignals.cc
rSparseVector.cc
rStreamIndex.cc
rStrings.cc
rTerrainBlockage2me.cc
rTerrainBlockage.cc
rTerrainBlockageLak.cc
rTime.cc
rTimeDuration.cc
rUnit.cc
rURL.cc
rValueCompressor.cc
rVolumeValueResolver.cc
rWebIndex.cc
rWebIndexWatcher.cc
rWebServer.cc
rXMLIndex.cc
rXYZ.cc
)

# Create RAPIO shared library
add_library(rapio SHARED
    "${SOURCE_FILES}"
)

# Add link to our custom build path.
link_directories(${RAPIO_INSTALL_LIB})

include_directories(. 
  ${RAPIO_INCLUDE_DIRS}
)

# Need this to add the BOOST setting or the rError.cc will complain about v2
add_definitions(-DBOOST_LOG_DYN_LINK)
##################################################################################################

# Link libraries
target_link_libraries(rapio
  ${RAPIO_THIRD_LIBRARIES}
)

# Install the RAPIO headers
set_target_properties(rapio PROPERTIES
  PUBLIC_HEADER "${HEADER_FILES}"
)
install(TARGETS rapio
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/rapiobase
  LIBRARY DESTINATION ${RAPIO_INSTALL_LIB}
)
