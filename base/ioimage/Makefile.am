AM_CPPFLAGS = -I../ -I@prefix@/include -I/usr/include/GraphicsMagick/
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK

lib_LTLIBRARIES =\
  librapioimage.la

librapioimage_la_LIBADD =\
   -L@prefix@/lib -lgrib2c -lboost_system -lGraphicsMagick++ -lgdal

librapioimage_la_SOURCES =\
  rImageDataTypeImp.cc\
  rIOImage.cc
   
pkginclude_HEADERS =\
  rImageDataTypeImp.h\
  rIOImage.h
