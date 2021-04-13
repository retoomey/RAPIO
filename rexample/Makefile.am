AM_CXXFLAGS = -I../base -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions  $(RAPIO_CFLAGS)

bin_PROGRAMS =\
 rsimplealg rtile

rsimplealg_SOURCES = rsimplealg.cc rSimpleAlg.cc
rsimplealg_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio 

rtile_SOURCES = rTile.cc
rtile_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio
