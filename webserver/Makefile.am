AM_CXXFLAGS = -I../base -std=c++11 -Wall -O2 -ftemplate-depth-50 -finline-functions  $(RAPIO_CFLAGS)

check_PROGRAMS =\
 servertest servertests

servertest_SOURCES = http_examples.cpp
servertest_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio 

servertests_SOURCES = https_examples.cpp
servertests_LDADD = \
   $(RAPIO_LIBS) -L../base/ -lrapio 
