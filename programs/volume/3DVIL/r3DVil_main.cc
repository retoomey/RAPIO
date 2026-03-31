#include "r3DVil.h"

using namespace rapio;

/** Run 3D Vil standalone.  VIL can also be called by FusionAlgs. */
int
main(int argc, char * argv[])
{
  VIL alg = VIL();

  alg.executeFromArgs(argc, argv);
} // main
