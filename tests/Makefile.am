AM_CPPFLAGS = -I../ -I@prefix@/include 
AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -ftemplate-depth-1024 -finline-functions -ggdb -g3 -DBOOST_LOG_DYN_LINK 

check_PROGRAMS =\
  testrapio

testrapio_SOURCES =\
   rTestMain.cc\
   rTestTime.cc\
   rTestURL.cc

testrapio_LDADD = \
   -L../base/ -L../../lib -lboost_system -lrapio -lcurl -lnetcdf -ludunits2 -lbz2 -ljasper -lpng -lboost_system -lboost_log -lboost_log_setup -lboost_thread -lpthread -lboost_filesystem -lboost_serialization -lboost_iostreams -lboost_unit_test_framework

