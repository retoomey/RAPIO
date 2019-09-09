AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions

bin_PROGRAMS =\
 rsimplealg

rsimplealg_SOURCES = rsimplealg.cc rSimpleAlg.cc
rsimplealg_LDADD = \
   -L../base/ -L../../lib -lrapio -lcurl -lnetcdf -ludunits2 -lbz2 -ljasper -lpng
