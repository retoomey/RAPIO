AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -lrt -ftemplate-depth-50 -finline-functions -DBOOST_LOG_DYN_LINK

bin_PROGRAMS =\
 rPython

rPython_SOURCES = rPython.cc
rPython_LDADD = \
   -L../base/ -L../../lib -lboost_system -lrapio -lcurl -lnetcdf -ludunits2 -lbz2 -ljasper -lpng -lboost_system -lboost_log -lboost_log_setup -lboost_thread -lpthread -lboost_filesystem -lboost_serialization -lboost_iostreams
