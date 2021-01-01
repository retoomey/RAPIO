AM_CPPFLAGS = -I../ -I@prefix@/include
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK

lib_LTLIBRARIES =\
  librapiogdal.la

librapiogdal_la_LIBADD =\
   -L@prefix@/lib -lgrib2c -lboost_system -lgdal

librapiogdal_la_SOURCES =\
  rGDALDataTypeImp.cc\
  rIOGDAL.cc
   
pkginclude_HEADERS =\
  rGDALDataTypeImp.h\
  rIOGDAL.h
