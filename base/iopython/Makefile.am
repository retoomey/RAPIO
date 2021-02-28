AM_CPPFLAGS = -I../ -I@prefix@/include -I/usr/include/python3.9
AM_CXXFLAGS = -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -DBOOST_LOG_DYN_LINK

librapiopython_la_LIBADD =\
   -L@prefix@/lib 

#if HAVE_PYTHON
#  AM_CPPFLAGS += $(MAGICK_CFLAGS)
# Pass on conditional to code
#  AM_CXXFLAGS += -DHAVE_MAGICK
#  librapiopython_la_LIBADD += $(MAGICK_LIBS)
#endif

lib_LTLIBRARIES =\
  librapiopython.la

librapiopython_la_SOURCES =\
  rIOPython.cc
   
pkginclude_HEADERS =\
  rIOPython.h
