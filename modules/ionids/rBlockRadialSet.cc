#include "rBlockRadialSet.h"
#include <rError.h>

using namespace rapio;

void
BlockRadialSet::read(StreamBuffer& b)
{
  fLogInfo("Reading Radial block");
  checkDivider(b);

  // Check data length
  int lengthOfDataLayer = b.readInt();

  if (lengthOfDataLayer == 0) {
    throw std::runtime_error("Length of Radial Data is zero");
  }
  fLogInfo("length {} expect 434174", lengthOfDataLayer);

  myPacketCode = b.readShort();

  // FIXME: Packet code info table or not?

  fLogInfo("RadialSet Packet Code {}", myPacketCode);
  switch (myPacketCode) {
      case 1: {
        // Skip rest for null product
        size_t length = lengthOfDataLayer - sizeof(myPacketCode);
        b.forward(length);
        throw std::runtime_error("Product code 1 - NULL product unimplemented.");
      }
      break;
      case 28:
        // Skipping rpc.h products.
        throw std::runtime_error("Product code 28 - XDR compressed product unimplemented.");
        break;
      case -20705: // eh?
      case 16:
        fLogInfo("Looks like a RadialSet NIDS product.");
        break;
      default:
        fLogSevere("Unknown packet code {}", myPacketCode);
        throw std::runtime_error("Unsupported packet code");
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
  fLogInfo("Writing RadialSet Description block");

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

    // short numChunks = static_cast<short>(r.data.size());
    // b.writeShort(numChunks);
    b.writeShort(static_cast<short>(r.data.size()));

    b.writeShort(static_cast<short>(std::round(r.start_angle * 10.0f)));

    //    b.writeShort(r.start_angle*10.0);
    //    float temp = r.delta_angle*10.0;
    //    b.writeShort(temp);
    // Force round at end or you can get clipped values
    b.writeShort(static_cast<short>(std::round(r.delta_angle * 10.0f)));

    //
    // Humm stored as char or short, different right?
    // for (size_t gate = 0; gate < r.data.size(); gate+=2) {
    for (size_t gate = 0; gate < r.data.size(); gate++) {
      b.writeChar(r.data[gate]);
    }
  }
} // BlockRadialSet::write

void
BlockRadialSet::dump()
{
  fLogInfo("index first {}", myIndexFirstBin);
  fLogInfo("num range first {}", myNumRangeBin);
  fLogInfo("I center {}", myCenterOfSweepI);
  fLogInfo("J center {}", myCenterOfSweepJ);
  fLogInfo("scale {}", myScaleFactor);
  fLogInfo("num_radials {}", myNumRadials);
}
