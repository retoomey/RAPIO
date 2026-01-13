#include "rBlockProductSymbology.h"
#include <rError.h>

using namespace rapio;

void
BlockProductSymbology::read(StreamBuffer& b)
{
  fLogInfo("Reading Product Symbology block");

  checkDivider(b);

  myBlockID     = b.readShort();
  myBlockLength = b.readInt();
  myNumLayers   = b.readShort();
}

void
BlockProductSymbology::write(StreamBuffer& b)
{
  fLogInfo("Writing Product Symbology block");

  writeDivider(b);

  b.writeShort(myBlockID);
  b.writeInt(myBlockLength);
  b.writeShort(myNumLayers);
}

void
BlockProductSymbology::dump()
{
  fLogInfo("Block ID {} expect 1", myBlockID);              // block id
  fLogInfo("Block Length {} expect 434190", myBlockLength); // block length
  // Can have N layers like grib2, etc.
  fLogInfo("Num Layers {} expect 1", myNumLayers); // num layers
}
