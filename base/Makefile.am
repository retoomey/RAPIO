AM_CPPFLAGS = -I@prefix@/include
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK

lib_LTLIBRARIES =\
  librapio.la

# MRMS builder makes a lib folder one up from us with all the extra libraries
# we use.
librapio_la_LIBADD =\
   -L@prefix@/lib -lg2c_v1.6.0

librapio_la_SOURCES =\
  rRAPIOAlgorithm.cc\
  rRAPIOOptions.cc\
  rRAPIOData.cc\
  rColorTerm.cc\
  rCurlConnection.cc\
  rEventTimer.cc\
  rEventLoop.cc\
  rCompression.cc\
  rSignals.cc\
  rRecord.cc\
  rRecordQueue.cc\
  rRecordNotifier.cc\
  rIOIndex.cc\
  rURL.cc\
  rConfig.cc\
  rConfigDataFormat.cc\
  rConfigDirectoryMapping.cc\
  rIODataType.cc\
  rIndexType.cc\
  rError.cc\
  rIOXML.cc\
  rIOURL.cc\
  rOS.cc\
  rWebIndex.cc\
  rWebIndexWatcher.cc\
  rProcessTimer.cc\
  rXMLIndex.cc\
  rXYZ.cc\
  rIJK.cc\
  rConstants.cc\
  rLength.cc\
  rTimeDuration.cc\
  rTime.cc\
  rUnit.cc\
  rLL.cc\
  rLLH.cc\
  rStrings.cc\
  rBinaryTable.cc\
  rDataGrid.cc\
  rLatLonGrid.cc\
  rRadialSet.cc\
  rDataType.cc\
  rGribDataType.cc\
  rNetcdfDataType.cc\
  rFMLIndex.cc\
  rFMLRecordNotifier.cc\
  rIOWatcher.cc\
  rFAMWatcher.cc\
  rIONetcdf.cc\
  rIOGrib.cc\
  rNetcdfBinaryTable.cc\
  rNetcdfRadialSet.cc\
  rNetcdfLatLonGrid.cc\
  rOption.cc\
  rOptionList.cc\
  rAlgConfigFile.cc\
  rTest.cc\
  rTestTime.cc\
  rTestIndex.cc

pkginclude_HEADERS =\
  RAPIO.h\
  rRapio.h\
  rRAPIOAlgorithm.h\
  rRAPIOOptions.h\
  rRAPIOData.h\
  rColorTerm.h\
  rCurlConnection.h\
  rTest.h\
  rData.h\
  rAlgorithm.h\
  rEvent.h\
  rEventLoop.h\
  rEventTimer.h\
  rIO.h\
  rByteOrder.h\
  rFactory.h\
  rXMLIndex.h\
  rCompression.h\
  rURL.h\
  rProcessTimer.h\
  rWebIndex.h\
  rWebIndexWatcher.h\
  rSignals.h\
  rIOIndex.h\
  rIOURL.h\
  rError.h\
  rBuffer.h\
  rConfig.h\
  rConfigDataFormat.h\
  rConfigDirectoryMapping.h\
  rOS.h\
  rIODataType.h\
  rIOXML.h\
  rIndexType.h\
  rLLH.h\
  rXYZ.h\
  rIJK.h\
  rConstants.h\
  rArith.h\
  rLL.h\
  rTimeDuration.h\
  rUnit.h\
  rStrings.h\
  rBinaryIO.h\
  rLength.h\
  rTime.h\
  rDataStore.h\
  rDataStore2D.h\
  rDataType.h\
  rGribDataType.h\
  rNetcdfDataType.h\
  rDataGrid.h\
  rBinaryTable.h\
  rRadialSet.h\
  rLatLonGrid.h\
  rFMLIndex.h\
  rFMLRecordNotifier.h\
  rIOWatcher.h\
  rFAMWatcher.h\
  rIONetcdf.h\
  rIOGrib.h\
  rNetcdfBinaryTable.h\
  rNetcdfRadialSet.h\
  rNetcdfLatLonGrid.h\
  rOption.h\
  rOptionList.h\
  rSentinelDouble.h\
  rRecord.h\
  rRecordQueue.h\
  rRecordNotifier.h\
  rUtility.h\
  rAlgConfigFile.h\
  rTestTime.h\
  rTestIndex.h

