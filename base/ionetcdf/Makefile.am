AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK

lib_LTLIBRARIES =\
  librapionetcdf.la

librapionetcdf_la_LIBADD =\
   -L@prefix@/lib -lboost_system -lnetcdf

librapionetcdf_la_SOURCES =\
  rIONetcdf.cc\
  rNetcdfDataGrid.cc\
  rNetcdfBinaryTable.cc\
  rNetcdfRadialSet.cc\
  rNetcdfLatLonGrid.cc\
  rNetcdfDataTypeImp.cc
   
pkginclude_HEADERS =\
  rNetcdfDataTypeImp.h\
  rNetcdfDataGrid.h\
  rNetcdfBinaryTable.h\
  rNetcdfRadialSet.h\
  rNetcdfLatLonGrid.h
