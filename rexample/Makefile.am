AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions -DBOOST_LOG_DYN_LINK
#AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions

bin_PROGRAMS =\
 rsimplealg

rsimplealg_SOURCES = rsimplealg.cc rSimpleAlg.cc
rsimplealg_LDADD = \
   -L../base/ -L../../lib -lrapio -lcurl -lnetcdf -ludunits2 -lbz2 -ljasper -lpng -lboost_log -lboost_log_setup -lboost_thread -lpthread -lboost_filesystem
#   -L../base/ -L../../lib -lrapio -lcurl -lnetcdf -ludunits2 -lbz2 -ljasper -lpng  -lpthread
