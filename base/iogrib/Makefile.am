AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 $(GRIB_CFLAGS)

lib_LTLIBRARIES =\
  librapiogrib.la

librapiogrib_la_LIBADD =\
   -L@prefix@/lib $(GRIB_LIBS)

librapiogrib_la_SOURCES =\
  rGribDataTypeImp.cc\
  rIOGrib.cc
   
pkginclude_HEADERS =\
  rGribDataTypeImp.h\
  rIOGrib.h
