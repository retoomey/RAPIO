#include "r3DLayerAverage.h"

using namespace rapio;

/** Run 3DLayerAverage standalone. Can also be called dynamically by FusionAlgs. */
int
main(int argc, char * argv[])
{
  LayerAverageAlg alg;

  alg.executeFromArgs(argc, argv);
  return 0;
} // main
