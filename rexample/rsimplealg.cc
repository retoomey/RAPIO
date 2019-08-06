// An example of how to make and call a simple algorithm
// Robert Toomey
// June 2019

// Your simple algorithm.
#include <rSimpleAlg.h>

// Whatever namespace YOUR project will be part of.
// Suggestions for NSSL:
// wdssii --> wdssii part of mrms
// hmet --> hydrology applications
// mrms --> general mrms utilities
using namespace wdssii;

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  W2SimpleAlg alg = W2SimpleAlg();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
