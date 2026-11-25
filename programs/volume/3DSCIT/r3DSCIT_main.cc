#include "r3DSCIT.h"

using namespace rapio;

/** Run 3D SCIT standalone. SCIT can also be called by FusionAlgs. */
int
main(int argc, char * argv[])
{
  SCITAlg alg = SCITAlg();

  alg.executeFromArgs(argc, argv);
}
