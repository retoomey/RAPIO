#include "rBlockProductSymbology.h"
#include <rError.h>

using namespace rapio;

void
BlockProductSymbology::read(StreamBuffer& b)
{
  LogInfo("Reading Product Symbology block\n");

  checkDivider(b);

  myBlockID     = b.readShort();
  myBlockLength = b.readInt();
  myNumLayers   = b.readShort();
}

void
BlockProductSymbology::write(StreamBuffer& b)
{
  LogInfo("Writing Product Symbology block\n");

  writeDivider(b);

  b.writeShort(myBlockID);
  b.writeInt(myBlockLength);
  b.writeShort(myNumLayers);
}

void
BlockProductSymbology::dump()
{
  LogInfo("Block ID " << myBlockID << " expect 1\n");              // block id
  LogInfo("Block Length " << myBlockLength << " expect 434190\n"); // block length
  // Can have N layers like grib2, etc.
  LogInfo("Num Layers " << myNumLayers << " expect 1\n"); // num layers
}
