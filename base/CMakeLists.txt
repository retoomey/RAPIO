# Toomey Sept 2021
# RAPIO base cmake file
#

# Set variable of all headers we use
set(HEADER_FILES
  RAPIO.h
  rTerrainBlockage.h
  rArray.h
  rBinaryIO.h
  rColor.h
  rColorMap.h
  rDataFilter.h
  rDataTable.h
  rRapio.h
  rRAPIOAlgorithm.h
  rRAPIOOptions.h
  rRAPIOData.h
  rColorTerm.h
  rCurlConnection.h
  rData.h
  rAlgorithm.h
  rEvent.h
  rEventLoop.h
  rEventTimer.h
  rIO.h
  rFactory.h
  rStaticMethodFactory.h
  rXMLIndex.h
  rStreamIndex.h
  rURL.h
  rProcessTimer.h
  rWebIndex.h
  rWebIndexWatcher.h
  rWebServer.h
  rDirWatcher.h
  rSecure.h
  rSignals.h
  rIOIndex.h
  rIOURL.h
  rError.h
  rConfig.h
  rConfigIODataType.h
  rConfigLogging.h
  rConfigDirectoryMapping.h
  rConfigParamGroup.h
  rOS.h
  rIODataType.h
  rNetcdfDataType.h
  rGribDataType.h
  rImageDataType.h
  rGDALDataType.h
  rIOFile.h
  rIOXML.h
  rIOJSON.h
  rIndexType.h
  rLLH.h
  rXYZ.h
  rIJK.h
  rConstants.h
  rArith.h
  rLL.h
  rTimeDuration.h
  rUnit.h
  rStrings.h
  rBinaryIO.h
  rLength.h
  rTime.h
  rDataStore.h
  rDataStore2D.h
  rDataType.h
  rDataProjection.h
  rPTreeData.h
  rDataGrid.h
  rBinaryTable.h
  rRadialSet.h
  rRadialSetLookup.h
  rDataTypeHistory.h
  rElevationVolume.h
  rLatLonGrid.h
  rLatLonHeightGrid.h
  rFileIndex.h
  rFMLIndex.h
  rFMLRecordNotifier.h
  rEXERecordNotifier.h
  rHeartbeat.h
  rIOWatcher.h
  rFAMWatcher.h
  rEXEWatcher.h
  rOption.h
  rOptionList.h
  rSentinelDouble.h
  rRecord.h
  rRecordFilter.h
  rRecordQueue.h
  rRecordNotifier.h
  rUtility.h
  rAlgConfigFile.h
  rAWS.h
  rProject.h
  rNamedAny.h
)

# Set variable of all sources we use
set(SOURCE_FILES
  rTerrainBlockage.cc
  rArray.cc
  rBinaryIO.cc
  rColor.cc
  rColorMap.cc
  rDataFilter.cc
  rDataTable.cc
  rRAPIOAlgorithm.cc
  rRAPIOOptions.cc
  rRAPIOData.cc
  rColorTerm.cc
  rCurlConnection.cc
  rEventTimer.cc
  rEventLoop.cc
  rSecure.cc
  rSignals.cc
  rRecord.cc
  rRecordFilter.cc
  rRecordQueue.cc
  rRecordNotifier.cc
  rIOIndex.cc
  rURL.cc
  rConfig.cc
  rConfigDirectoryMapping.cc
  rConfigIODataType.cc
  rConfigLogging.cc
  rConfigParamGroup.cc
  rIODataType.cc
  rNetcdfDataType.cc
  rGribDataType.cc
  rImageDataType.cc
  rGDALDataType.cc
  rIndexType.cc
  rError.cc
  rIOFile.cc
  rIOXML.cc
  rIOJSON.cc
  rIOURL.cc
  rOS.cc
  rWebIndex.cc
  rWebIndexWatcher.cc
  rWebServer.cc
  rDirWatcher.cc
  rProcessTimer.cc
  rXMLIndex.cc
  rStreamIndex.cc
  rXYZ.cc
  rIJK.cc
  rConstants.cc
  rLength.cc
  rTimeDuration.cc
  rTime.cc
  rUnit.cc
  rLL.cc
  rLLH.cc
  rStrings.cc
  rBinaryTable.cc
  rDataGrid.cc
  rLatLonGrid.cc
  rLatLonHeightGrid.cc
  rRadialSet.cc
  rRadialSetLookup.cc
  rDataTypeHistory.cc
  rElevationVolume.cc
  rDataType.cc
  rDataProjection.cc
  rPTreeData.cc
  rFileIndex.cc
  rFMLIndex.cc
  rFMLRecordNotifier.cc
  rEXERecordNotifier.cc
  rHeartbeat.cc
  rIOWatcher.cc
  rFAMWatcher.cc
  rEXEWatcher.cc
  rOption.cc
  rOptionList.cc
  rAlgConfigFile.cc
  rAWS.cc
  rProject.cc
  ccronexpr.c
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
