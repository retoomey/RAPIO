AM_CXXFLAGS = -I../base -std=c++11 -Wall -O2 -lrt -ftemplate-depth-50 -finline-functions $(RAPIO_CFLAGS)

bin_PROGRAMS =\
 rPython

rPython_SOURCES = rPython.cc
rPython_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio
