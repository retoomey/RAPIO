#include "rBlockRadialSet.h"
#include <rError.h>

using namespace rapio;

void
BlockRadialSet::read(StreamBuffer& b)
{
  LogInfo("Reading Radial block\n");
  checkDivider(b);

  // Check data length
  int lengthOfDataLayer = b.readInt();

  if (lengthOfDataLayer == 0) {
    throw std::runtime_error("Length of Radial Data is zero\n");
  }
  LogInfo("length " << lengthOfDataLayer << " expect 434174\n");

  myPacketCode = b.readShort();

  // FIXME: Packet code info table or not?

  LogInfo("RadialSet Packet Code " << myPacketCode << "\n");
  switch (myPacketCode) {
      case 1: {
        // Skip rest for null product
        size_t length = lengthOfDataLayer - sizeof(myPacketCode);
        b.forward(length);
        throw std::runtime_error("Product code 1 - NULL product unimplemented.\n");
      }
      break;
      case 28:
        // Skipping rpc.h products.
        throw std::runtime_error("Product code 28 - XDR compressed product unimplemented.\n");
        break;
      case -20705: // eh?
      case 16:
        LogInfo("Looks like a RadialSet NIDS product.\n");
        break;
      default:
        LogSevere("Unknown packet code " << myPacketCode << "\n");
        throw std::runtime_error("Unsupported packet code \n");
        return;

        break;
  }

  // Reading packet 1, 0xAF1F (-20705) or a 0x10 (16)
  //
  myIndexFirstBin  = b.readShort();
  myNumRangeBin    = b.readShort();
  myCenterOfSweepI = b.readShort();
  myCenterOfSweepJ = b.readShort();
  myScaleFactor    = b.readShort();
  myNumRadials     = b.readShort();

  // This is the data format used to store the set of radials
  bool inShorts = (myPacketCode != 16);

  // So we loop over the number of radials
  myRadials.resize(myNumRadials);
  for (int i = 0; i < myNumRadials; i++) {
    auto& r = myRadials[i];

    // -----------------------------------------
    // Start radial data
    //
    short numChunks = b.readShort();

    short tempShort = b.readShort();
    if (tempShort >= 3600) { tempShort -= 3600; } // or mod?
    r.start_angle = static_cast<float>(tempShort * 0.1);

    // So I guess individual radials can store differently?
    tempShort     = b.readShort();
    r.inShorts    = tempShort;
    r.delta_angle = static_cast<float>(tempShort * 0.1);

    // Data might be just per char, or double char (big endian shorts)
    // FIXME readChar(length) or vector maybe?
    for (size_t j = 0; j < numChunks; j++) {
      r.data.push_back(b.readChar());
    }
  }
} // BlockRadialSet::read

void
BlockRadialSet::write(StreamBuffer& b)
{
  LogInfo("Writing RadialSet Description block\n");

  writeDivider(b);
  b.writeInt(10000); // length of data?

  b.writeShort(myPacketCode);

  b.writeShort(myIndexFirstBin);
  b.writeShort(myNumRangeBin);
  b.writeShort(myCenterOfSweepI);
  b.writeShort(myCenterOfSweepJ);
  b.writeShort(myScaleFactor);
  b.writeShort(myNumRadials);

  // RadialSet write...
  for (int i = 0; i < myNumRadials; i++) {
    auto& r = myRadials[i];
    LogInfo("would try to write a radial " << i << "\n");
  }
}

void
BlockRadialSet::dump()
{
  LogInfo("index first " << myIndexFirstBin << "\n");
  LogInfo("num range first " << myNumRangeBin << "\n");
  LogInfo("I center " << myCenterOfSweepI << "\n");
  LogInfo("J center " << myCenterOfSweepJ << "\n");
  LogInfo("scale " << myScaleFactor << "\n");
  LogInfo("num_radials " << myNumRadials << "\n");
}
