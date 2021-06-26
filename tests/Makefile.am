AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -I../base -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 $(RAPIO_CFLAGS)

check_PROGRAMS =\
  testrapio rTestNetcdf

testrapio_SOURCES =\
   rTestMain.cc\
   rTestTime.cc\
   rTestURL.cc\
   rTestIODataType.cc

testrapio_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio -lboost_unit_test_framework

rTestNetcdf_SOURCES = rTestNetcdf.cc
rTestNetcdf_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio

