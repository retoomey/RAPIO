# link to our custom build path.
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

include_directories(. ${CMAKE_INSTALL_PREFIX}/include)

# Need this to add the BOOST setting or the rError.cc will complain about v2
add_definitions(-DBOOST_LOG_DYN_LINK)

set(HEADER_FILES
  RAPIO.h
  rArray.h
  rColor.h
  rColorMap.h
  rDataFilter.h
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

set(SOURCE_FILES
  rArray.cc
  rColor.cc
  rColorMap.cc
  rDataFilter.cc
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

add_library(rapio SHARED
    "${SOURCE_FILES}"
)

# container /usr/include/udunits2
# /usr/lib64/libudunits2.so

##################################################################################################
# Hunt for udunits2, priority is the MRMS third party folder
find_path( UDUNITS_HEADERS NAMES udunits.h
  HINTS ${CMAKE_PREFIX_INSTALL}/include /usr/include/udunits2
)

find_library( UDUNITS_LIBS NAMES udunits2
  PATHS ${CMAKE_PREFIX_INSTALL}/lib /usr/lib64
)

if (UDUNITS_LIBS AND UDUNITS_HEADERS)
  include_directories(${UDUNITS_HEADERS})
  message(STATUS "Udunits2: ${UDUNITS_HEADERS} ${UDUNITS_LIBS}")
else(
  message(FATAL_ERROR "Couldn't find udunits2 library.")
)
endif(UDUNITS_LIBS AND UDUNITS_HEADERS)
##################################################################################################

# Yeah I know we need to check these libraries
# I'm just starting out with cmake
target_link_libraries(rapio
  ${UDUNITS_LIBS}
  proj
  curl
  bz2
  jasper
  png
  pthread
  boost_system
  boost_log
  boost_log_setup
  boost_thread
  boost_filesystem
  boost_serialization
  boost_iostreams
  boost_timer
)

set_target_properties(rapio PROPERTIES
  PUBLIC_HEADER "${HEADER_FILES}"
)

# Install the RAPIO headers
install(TARGETS rapio
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/rapiobase
)

# Create the modules.  They could be built on the top layer actually?
add_subdirectory(ionetcdf)
add_subdirectory(ioimage)
add_subdirectory(iogdal)
add_subdirectory(iogrib)
add_subdirectory(iopython)
