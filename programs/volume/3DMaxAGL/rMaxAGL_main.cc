#include "rMaxAGL.h"

using namespace rapio;

/** Run MaxAGL standalone.  MaxAGL can also be called by FusionAlgs. */
int
main(int argc, char * argv[])
{
  MaxAGL alg = MaxAGL();

  alg.executeFromArgs(argc, argv);
} // main
