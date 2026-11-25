#include "r3DHDA.h"

using namespace rapio;

/** Run 3D HDA standalone. HDA can also be called by FusionAlgs. */
int
main(int argc, char * argv[])
{
  HDAAlg alg = HDAAlg();

  alg.executeFromArgs(argc, argv);
}
