AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK

lib_LTLIBRARIES =\
  librapiogrib.la

librapiogrib_la_LIBADD =\
   -L@prefix@/lib -lgrib2c -lboost_system 

librapiogrib_la_SOURCES =\
  rGribDataTypeImp.cc\
  rIOGrib.cc
   
pkginclude_HEADERS =\
  rGribDataTypeImp.h\
  rIOGrib.h
