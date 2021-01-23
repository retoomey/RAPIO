AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -DBOOST_LOG_DYN_LINK

librapioimage_la_LIBADD =\
   -L@prefix@/lib 

if HAVE_MAGICK
  AM_CPPFLAGS += $(MAGICK_CFLAGS)
# Pass on conditional to code
  AM_CXXFLAGS += -DHAVE_MAGICK
  librapioimage_la_LIBADD += $(MAGICK_LIBS)
endif

lib_LTLIBRARIES =\
  librapioimage.la

librapioimage_la_SOURCES =\
  rImageDataTypeImp.cc\
  rIOImage.cc
   
pkginclude_HEADERS =\
  rImageDataTypeImp.h\
  rIOImage.h
