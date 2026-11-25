#include "r3DColumnStats.h"

using namespace rapio;

/** Run 3DColumnStats standalone. Can also be dynamically loaded by FusionAlgs. */
int
main(int argc, char * argv[])
{
  ColumnStatsAlg alg;

  alg.executeFromArgs(argc, argv);
}
