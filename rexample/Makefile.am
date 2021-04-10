AM_CXXFLAGS = -I../base -I../../include -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions -DBOOST_LOG_DYN_LINK

bin_PROGRAMS =\
 rsimplealg rtile

rsimplealg_SOURCES = rsimplealg.cc rSimpleAlg.cc
rsimplealg_LDADD = \
   -L../base/ -L../../lib -lrapio 

rtile_SOURCES = rTile.cc
rtile_LDADD = \
   -L../base/ -L../../lib -lrapio 
