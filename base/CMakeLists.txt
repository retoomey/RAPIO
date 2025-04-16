# Toomey Sept 2021
# RAPIO base cmake file
#

# Set variable of all headers we use
# We 'could' file(GLOB HEADER_FILES "*.h") but
# this means when we add a file we have to force cmake
# to recognize the change so for now we'll hard list the files
# ls -1 *.h
set(RELATIVE_HEADERS
ccronexpr.h
rAlgConfigFile.h
rAlgorithm.h
rArith.h
rArray.h
rArrayAlgorithm.h
datatype/rAttributeDataType.h
rAWS.h
rBimap.h
rBinaryIO.h
datatype/rBinaryTable.h
rBitset.h
rBoostConnection.h
rColor.h
rColorMap.h
rColorTerm.h
config/rConfigColorMap.h
config/rConfigDirectoryMapping.h
config/rConfig.h
config/rConfigIODataType.h
config/rConfigLogging.h
config/rConfigParamGroup.h
config/rConfigRadarInfo.h
config/rConfigRecord.h
rConstants.h
rCurlConnection.h
rDataArray.h
rDataFilter.h
datatype/rDataGrid.h
rData.h
rDataProjection.h
rDataStore2D.h
rDataStore3D.h
rDataStore.h
datatype/rDataTable.h
datatype/rDataType.h
rDataTypeHistory.h
rDimensionMapper.h
rDirWalker.h
watcher/rDirWatcher.h
rElevationVolume.h
rError.h
rEvent.h
rEventLoop.h
rEventTimer.h
notifier/rEXERecordNotifier.h
watcher/rEXEWatcher.h
rFactory.h
watcher/rFAMWatcher.h
index/rFileIndex.h
index/rFMLIndex.h
notifier/rFMLRecordNotifier.h
datatype/rGDALDataType.h
datatype/rGribDataType.h
rHeartbeat.h
rIJK.h
datatype/rImageDataType.h
index/rIndexType.h
rIODataType.h
rIOFile.h
rIO.h
index/rIOIndex.h
rIOJSON.h
rIOURL.h
watcher/rIOWatcher.h
rIOXML.h
datatype/rLatLonArea.h
datatype/rLatLonGrid.h
datatype/rLatLonGridProjection.h
datatype/rLatLonHeightGrid.h
datatype/rLatLonHeightGridProjection.h
rLength.h
rLLCoverageArea.h
rLL.h
rLLH.h
datatype/rLLHGridN2D.h
rNamedAny.h
datatype/rNetcdfDataType.h
rNetwork.h
rOption.h
rOptionList.h
rOS.h
rPartitionInfo.h
rProcessTimer.h
rProject.h
datatype/rPTreeData.h
datatype/rRadialSet.h
datatype/rRadialSetIterator.h
datatype/rRadialSetProjection.h
rRAPIOAlgorithm.h
rRAPIOData.h
rRapio.h
datatype/rMultiDataType.h
datatype/rDataTable.h
rRAPIOOptions.h
plugin/rRAPIOPlugin.h
plugin/rPluginHeartbeat.h
plugin/rPluginWebserver.h
plugin/rPluginNotifier.h
plugin/rPluginIngestor.h
plugin/rPluginRealTime.h
plugin/rPluginRecordFilter.h
plugin/rPluginProductOutput.h
plugin/rPluginProductOutputFilter.h
plugin/rPluginVolumeValueResolver.h
plugin/rPluginTerrainBlockage.h
plugin/rPluginVolume.h
plugin/rPluginPartition.h
rRAPIOProgram.h
rRecordFilter.h
rRecord.h
notifier/rRecordNotifier.h
rRecordQueue.h
rSecure.h
rSentinelDouble.h
rSignals.h
rSparseVector.h
rStaticMethodFactory.h
index/rStreamIndex.h
rStrings.h
rTerrainBlockage2me.h
rTerrainBlockage.h
rTerrainBlockageLak.h
rThreadGroup.h
rTimeDuration.h
rTime.h
rUnit.h
rURL.h
rUtility.h
rValueCompressor.h
rVolumeValue.h
rVolumeValueResolver.h
index/rWebIndex.h
watcher/rWebIndexWatcher.h
rWebServer.h
index/rXMLIndex.h
rXYZ.h
)

# Set variable of all sources we use
# ls -1 *.c *.cc
set(SOURCE_FILES
ccronexpr.c
rAlgConfigFile.cc
rArray.cc
rArrayAlgorithm.cc
datatype/rAttributeDataType.cc
rAWS.cc
rBimap.cc
rBinaryIO.cc
datatype/rBinaryTable.cc
rBitset.cc
rBoostConnection.cc
rColor.cc
rColorMap.cc
rColorTerm.cc
config/rConfig.cc
config/rConfigColorMap.cc
config/rConfigDirectoryMapping.cc
config/rConfigIODataType.cc
config/rConfigLogging.cc
config/rConfigParamGroup.cc
config/rConfigRadarInfo.cc
config/rConfigRecord.cc
rConstants.cc
rCurlConnection.cc
rDataArray.cc
rDataFilter.cc
datatype/rDataGrid.cc
rDataProjection.cc
datatype/rMultiDataType.cc
datatype/rDataTable.cc
datatype/rDataType.cc
rDataTypeHistory.cc
rDimensionMapper.cc
rDirWalker.cc
watcher/rDirWatcher.cc
rElevationVolume.cc
rError.cc
rEventLoop.cc
rEventTimer.cc
notifier/rEXERecordNotifier.cc
watcher/rEXEWatcher.cc
watcher/rFAMWatcher.cc
index/rFileIndex.cc
index/rFMLIndex.cc
notifier/rFMLRecordNotifier.cc
datatype/rGDALDataType.cc
datatype/rGribDataType.cc
rHeartbeat.cc
rIJK.cc
datatype/rImageDataType.cc
index/rIndexType.cc
rIODataType.cc
rIOFile.cc
index/rIOIndex.cc
rIOJSON.cc
rIOURL.cc
watcher/rIOWatcher.cc
rIOXML.cc
datatype/rLatLonArea.cc
datatype/rLatLonGrid.cc
datatype/rLatLonGridProjection.cc
datatype/rLatLonHeightGrid.cc
datatype/rLatLonHeightGridProjection.cc
rLength.cc
rLL.cc
rLLCoverageArea.cc
rLLH.cc
datatype/rLLHGridN2D.cc
rNamedAny.cc
datatype/rNetcdfDataType.cc
rNetwork.cc
rOption.cc
rOptionList.cc
rOS.cc
rPartitionInfo.cc
rProcessTimer.cc
rProject.cc
datatype/rPTreeData.cc
datatype/rRadialSet.cc
datatype/rRadialSetIterator.cc
datatype/rRadialSetProjection.cc
rRAPIOAlgorithm.cc
rRAPIOData.cc
rRAPIOOptions.cc
plugin/rRAPIOPlugin.cc
plugin/rPluginHeartbeat.cc
plugin/rPluginWebserver.cc
plugin/rPluginNotifier.cc
plugin/rPluginIngestor.cc
plugin/rPluginRealTime.cc
plugin/rPluginRecordFilter.cc
plugin/rPluginProductOutput.cc
plugin/rPluginProductOutputFilter.cc
plugin/rPluginVolumeValueResolver.cc
plugin/rPluginTerrainBlockage.cc
plugin/rPluginVolume.cc
plugin/rPluginPartition.cc
rRAPIOProgram.cc
rRecord.cc
rRecordFilter.cc
notifier/rRecordNotifier.cc
rRecordQueue.cc
rSecure.cc
rSignals.cc
rSparseVector.cc
index/rStreamIndex.cc
rStrings.cc
rTerrainBlockage2me.cc
rTerrainBlockage.cc
rTerrainBlockageLak.cc
rTime.cc
rThreadGroup.cc
rTimeDuration.cc
rUnit.cc
rURL.cc
rValueCompressor.cc
rVolumeValueResolver.cc
index/rWebIndex.cc
watcher/rWebIndexWatcher.cc
rWebServer.cc
index/rXMLIndex.cc
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
  # Go ahead and make our linked libraries public for now.  In theory,
  # would be better some private
  PUBLIC ${RAPIO_THIRD_LIBRARIES}
  # Explicitly add public dlsys for any binaries c
  # calling dynamic loading methods.  rMakeIndex calls factory creation
  PUBLIC dl
)

# Declare the RAPIO headers for installation
target_include_directories(rapio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/rapio>
)

# Make absolute for top level install
foreach(hdr IN LISTS RELATIVE_HEADERS)
  list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${hdr}")
endforeach()
# Add the webserver header from webserver
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/server_http.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/asio_compatibility.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/mutex.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/utility.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/status_code.hpp")

# Add the created headers
list(APPEND HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/RAPIO.h")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/config.h")

set_target_properties(rapio PROPERTIES
  PUBLIC_HEADER "${HEADER_FILES}"
)
