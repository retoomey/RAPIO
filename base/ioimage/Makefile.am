# Pass on conditional to code
if HAVE_MAGICK
  AM_CPPFLAGS = -I../ -I@prefix@/include $(MAGICK_CFLAGS)
  AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK -DHAVE_MAGICK
  librapioimage_la_LIBADD =\
   -L@prefix@/lib -lboost_system $(MAGICK_LIBS) 
else
  AM_CPPFLAGS = -I../ -I@prefix@/include 
  AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK
  librapioimage_la_LIBADD =\
   -L@prefix@/lib -lboost_system 
endif


lib_LTLIBRARIES =\
  librapioimage.la

librapioimage_la_SOURCES =\
  rImageDataTypeImp.cc\
  rIOImage.cc
   
pkginclude_HEADERS =\
  rImageDataTypeImp.h\
  rIOImage.h
