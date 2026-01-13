#include "rBlockProductDesc.h"
#include "rNIDSUtil.h"
#include <rError.h>

#include "rConfigNIDSInfo.h"

using namespace rapio;

void
BlockProductDesc::read(StreamBuffer& b)
{
  fLogInfo("Reading Product Description block");

  checkDivider(b);

  // Read Location
  float latDegs   = b.readInt() / 1000.0;
  float lonDegs   = b.readInt() / 1000.0;
  float heightKMs = b.readShort() / 3280.74; // Feet to KMs

  myLocation = LLH(latDegs, lonDegs, heightKMs);

  // Read some fields
  myProductCode = b.readShort();
  myOpMode      = b.readShort();
  myVCP         = b.readShort();
  mySeqNumber   = b.readShort();
  myVolScanNum  = b.readShort();

  // Get the time fields
  short volScanDate    = b.readShort();
  int volScanStartTime = b.readInt();
  short genDate        = b.readShort();
  int genTime = b.readInt();

  // Get time fields
  // myVolStartTime = Time::SecondsSinceEpoch(volScanStartTime + ((volScanDate - 1) * 24 * 3600));
  myVolStartTime = NIDSUtil::getTimeFromNIDSTime(volScanDate, volScanStartTime);

  // Time oldmyVolStartTime = Time::SecondsSinceEpoch(volScanStartTime + ((volScanDate - 1) * 24 * 3600));
  if (genDate <= 1) {
    myGenTime = myVolStartTime;
  } else {
    // myGenTime = Time::SecondsSinceEpoch(genTime + ((genDate - 1) * 24 * 3600));
    myGenTime = NIDSUtil::getTimeFromNIDSTime(genDate, genTime);
  }

  // Read some more fields
  myDeps[0] = b.readShort();
  myDeps[1] = b.readShort();
  myElevNum = b.readShort();
  myDeps[2] = b.readShort();
  for (size_t i = 0; i < 16; ++i) {
    myDataThresholds[i] = b.readShort();
  }
  for (size_t i = 3; i < 10; i++) {
    myDeps[i] = b.readShort();
  }
  myNumMaps         = b.readShort();
  mySymbologyOffset = b.readInt();
  myGraphicOffset   = b.readInt();
  myTabularOffset   = b.readInt();
  dump();
} // BlockProductDesc::read

void
BlockProductDesc::write(StreamBuffer& b)
{
  fLogInfo("Writing Product Description block");

  writeDivider(b);

  // Write Location
  b.writeInt(myLocation.getLatitudeDeg() * 1000.0);
  b.writeInt(myLocation.getLongitudeDeg() * 1000.0);
  b.writeShort(myLocation.getHeightKM() * 3280.74);

  // Read some fields
  b.writeShort(myProductCode);
  b.writeShort(myOpMode);
  b.writeShort(myVCP);
  b.writeShort(mySeqNumber);
  b.writeShort(myVolScanNum);

  // Write the time fields
  auto epoch = myVolStartTime.getSecondsSinceEpoch();

  b.writeShort(epoch / 86400 + 1);
  b.writeInt(epoch % 86400);
  epoch = myGenTime.getSecondsSinceEpoch();
  b.writeShort(epoch / 86400 + 1);
  b.writeInt(epoch % 86400);

  // Write some more fields
  b.writeShort(myDeps[0]);
  b.writeShort(myDeps[1]);
  b.writeShort(myElevNum);
  b.writeShort(myDeps[2]);
  for (size_t i = 0; i < 16; ++i) {
    b.writeShort(myDataThresholds[i]);
  }
  for (size_t i = 3; i < 10; i++) {
    b.writeShort(myDeps[i]);
  }
  b.writeShort(myNumMaps);
  b.writeInt(mySymbologyOffset);
  b.writeInt(myGraphicOffset);
  b.writeInt(myTabularOffset);
  dump();
} // BlockProductDesc::write

void
BlockProductDesc::dump()
{
  fLogInfo("Location: {}", myLocation);
  fLogInfo("Product Code {}", myProductCode);
  fLogInfo("Op mode {}", myOpMode);
  fLogInfo("VCP {}", myVCP);
  fLogInfo("SEQ_NUM ", mySeqNumber);
  fLogInfo("VOL_SCAN_NUM {}", myVolScanNum);

  fLogInfo("VOL_START_TIME {}", myVolStartTime);
  fLogInfo("GEN_TIME {}", myGenTime);

  // So these values depend on the product type, right?
  // basically specials.  So radial set could read them.
  // Basically dedicate to product code lookup.

  fLogInfo("DEP01 {}", myDeps[0]);
  fLogInfo("DEP02 {}", myDeps[1]);
  fLogInfo("DEP03 (elevAngle) {}", myDeps[2]);

  fLogInfo("Elevation num {}", myElevNum);

  // 16 thresholds whatever those are.
  for (size_t i = 0; i < 16; ++i) {
    fLogInfo("Thres {} = {}", i, myDataThresholds[i]);
  }

  fLogInfo("DEP04 {}", myDeps[3]);
  fLogInfo("DEP05 {}", myDeps[4]);
  fLogInfo("DEP06 {}", myDeps[5]);
  fLogInfo("DEP07 {}", myDeps[6]);
  fLogInfo("DEP08 {}", myDeps[7]);
  fLogInfo("DEP09 {}", myDeps[8]);
  fLogInfo("DEP10 {}", myDeps[9]);

  uint32_t uncompBuffLen = ( myDeps[8] << 16) | (myDeps[9] & 0xffff);

  fLogInfo("----> Uncompressed length is {}", uncompBuffLen);
  fLogInfo("NUMMAPS {}", myNumMaps);

  // These look into the file at other blocks I guess
  fLogInfo("Symbology OFF {}", mySymbologyOffset);
  fLogInfo("Graphic OFF {}", myGraphicOffset);
  fLogInfo("Tab OFF {}", myTabularOffset);
} // BlockProductDesc::dump

void
BlockProductDesc::setDecodeMethod2(float scale, float offset,
  int leading_flags, int size)
{
  union { uint16_t i[2];
          float    f;
  } u;

  u.f = scale;
  myDataThresholds[0] = u.i[1];
  myDataThresholds[1] = u.i[0];

  u.f = offset;
  myDataThresholds[2] = u.i[1];
  myDataThresholds[3] = u.i[0];

  myDataThresholds[4] = 0;
  myDataThresholds[5] = size;
  myDataThresholds[6] = leading_flags;
  for (size_t i = 7; i < 16; i++) {
    myDataThresholds[i] = 0;
  }
}

void
BlockProductDesc::decodeMethod2(std::vector<float>& a) const
{
  // Method 2 no scale  required.
  // Original product 159, 161, 163, 170, 172, 173, 174, 175, 176

  fLogInfo("Threshold decode method 2.");
  auto _code = myProductCode; // FIXME: Still some code checks here

  /* for these products data level codes 0 and 1 correspond to "below
   * threshold" and "range folded", respectively.  Data levels 2-255
   * relate to the data in physical units via linear relationship, except
   * for product 163 which stops at data level 243.  The Scale and Offset
   * used in the equation (F = (N - OFFSET) / SCALE) where N is the integer
   * data value and F is the resulting floating point value, to relate the
   * integer data values to physical units are ANSI/IEEE standard 754-1985
   * floating point values. Halfwords 31 and 32 contain the Scale; 33-34
   * the Offset. */

  union { uint16_t i[2];
          float    f;
  } u;
  assert(sizeof(float) == 4);
  assert(sizeof(u) == 4);

  u.i[0] = myDataThresholds[1];
  u.i[1] = myDataThresholds[0];
  const float scale = u.f;

  u.i[0] = myDataThresholds[3];
  u.i[1] = myDataThresholds[2];
  const float offset = u.f;

  // FIXME: Still have some hardcoded products here that might be
  // movable to the info xml

  // The "magic number" of 65536 is defined in ICD 2620003P for
  // message 176 (Digital Instantaneous Precipitation Rate)
  const int leading_flags = myDataThresholds[6];
  const int numValues     = (_code == 176) ? 65536 :
    (myDataThresholds[5] + leading_flags);

  a.reserve(numValues + 2);
  int n = 0;

  if (_code != 176) {
    // Don't need these for 176.
    a.push_back(Constants::MissingData);
    n++;
    if ( (_code == 159) || (_code == 161) || (_code == 163)) {
      a.push_back(Constants::RangeFolded);
      n++;
    }
  }
  for (; n < numValues; ++n) {
    float f = ( n - offset ) / scale;

    // --------------------------------------------------------------
    // Unit conversion specials.  We only have a single unit in the info
    // right now.  Could do a source/destination general conversion.
    // Products 170, 172, 173, 174 and 175 are in 0.01 inches, not inches
    if ( (_code == 170) ||
      (_code == 172) ||
      (_code == 173) ||
      (_code == 174) ||
      (_code == 175) )
    {
      f /= 100.0; // convert to inches
    }
    // --------------------------------------------------------------
    a.push_back(f);
  }
} // BlockProductDesc::decodeMethod2

void
BlockProductDesc::decodeMethod3(std::vector<float>& a, int min, int delta, int num_data_level) const
{
  fLogInfo("Threshold decode method 3.");
  // Original product 32, 94, 153, 180, 182, 186
  a.push_back(Constants::MissingData); // below threshold
  a.push_back(Constants::MissingData); // missing

  float val = min;

  for (int i = 0; i < num_data_level; ++i) {
    a.push_back(val);
    val += delta;
  }
}

void
BlockProductDesc::decodeMethod4(std::vector<float>& a, int min, int delta, int num_data_level) const
{
  fLogInfo("Threshold decode method 4.");
  // Original product 93, 99, 154
  a.push_back(Constants::MissingData); // below threshold
  a.push_back(Constants::RangeFolded); // range folded  (only difference from method2)

  float val = min;

  for (int i = 0; i < num_data_level; ++i) {
    a.push_back(val);
    val += delta;
  }
}

void
BlockProductDesc::decodeMethod5(std::vector<float>& a, int min, int delta, int num_data_level) const
{
  fLogInfo("Threshold decode method 5.");
  // Original product 155
  a.push_back(Constants::MissingData); // below threshold
  a.push_back(Constants::RangeFolded); // range folded

  a.resize(129 + num_data_level); // 129 is the first bin for this product
  float val = min;

  for (int i = 0; i < num_data_level; ++i) {
    a[ 129 + i ] = val;
    val += delta;
  }
}

void
BlockProductDesc::decodeMethod6(std::vector<float>& a, int min, int delta, int num_data_level) const
{
  fLogInfo("Threshold decode method 6.");
  // Original product 33
  a.push_back(Constants::MissingData);
  for (int i = 0; i < num_data_level; i++) {
    a.push_back(min + delta * i * 250.0 / num_data_level);
  }
  a.push_back(Constants::MissingData);
}

void
BlockProductDesc::decodeMethod7(std::vector<float>& a, int min, int delta, int num_data_level) const
{
  fLogInfo("Threshold decode method 7.");
  // Original product 81
  a.push_back(Constants::MissingData);
  for (int i = 0; i < num_data_level; i++) {
    a.push_back(min + delta * i * 256.0 / num_data_level);
  }
  a.push_back(Constants::MissingData);
}

void
BlockProductDesc::decodeMethod1(std::vector<float>& a) const
{
  fLogInfo("Threshold decode method 1, default.");
  // Fallthrough
  for (int i = 0; i < (signed) myDataThresholds.size(); i++) {
    a.push_back(DecodeThresholds(myDataThresholds[i]));
  }
}

void
BlockProductDesc:: getDecodedThresholds(std::vector<float>& a) const
{
  auto _code        = myProductCode;
  const auto info   = ConfigNIDSInfo::getNIDSInfo(_code);
  int decode        = info.getDecode();
  auto minValue     = info.getMin();
  auto increase     = info.getIncrease();
  const float min   = (minValue != -1) ? (float) myDataThresholds[0] / minValue : 0;
  const float delta = (minValue != -1) ? (float) myDataThresholds[1] / increase : 0;
  const int levels  = (minValue != -1) ? myDataThresholds[2] : 0;

  fLogSevere("Min, delta, levels {}, {}, {}", min, delta, levels);
  switch (decode) {
      default:
        fLogSevere("Unknown decode method {}, trying default (1)", decode);
      // fallthrough to case 1
      case 1: return decodeMethod1(a);

      case 2: return decodeMethod2(a);

      case 3: return decodeMethod3(a, min, delta, levels);

      case 4: return decodeMethod4(a, min, delta, levels);

      case 5: return decodeMethod5(a, min, delta, levels);

      case 6: return decodeMethod6(a, min, delta, levels);

      case 7: return decodeMethod7(a, min, delta, levels);
  }
}

float
BlockProductDesc::DecodeThresholds(short int a) const
{
  short n         = (sizeof(short int) * CHAR_BIT ) - 1;
  short int mask1 = 1 << n; // mask1 = 10000000 00000000
  short int mask2 = 255;    // mask2 = 00000000 11111111

  if ( ((a & mask1 ) >> n ) == 0) { // bit 0 is set to zero
    return getValue(a);
  } else { // bit 0 is set to one
    short int value = a & mask2;
    if ( (value == 0) || ( value == 2) ) {
      return (float) Constants::MissingData;
    }
    if (value == 3) {
      return Constants::RangeFolded;
    }
    if (value == 1) {
      return (float)  myDeps[3]; // max value
    }
    return getValue(a);
  }
}

float
BlockProductDesc::getValue(short int a) const
{
  short int mask = 255;      // mask = 00000000 11111111
  float rtv      = a & mask; // get the lower byte

  short bit1 = a & 16384; // 16384 = 01000000 00000000

  if (bit1 == 16384) {
    return rtv / 100.0;
  }

  short bit2 = a & 8192; // 8192 = 00100000 00000000

  if (bit2 == 8192) {
    return  ( rtv / 20.0 );
  }

  short bit3 = a & 4096; // 4096 = 00010000 00000000

  if (bit3 == 4096) {
    return  ( rtv / 10.0 );
  }

  short bit4 = a & 2048; // 2048 = 00001000 00000000

  if (bit4 == 2048) {
    return  ( rtv + 0.5 );
  }

  short bit5 = a & 1024; // 11024 = 00000100 00000000

  if (bit5 == 1024) {
    return  ( rtv - 0.5 );
  }

  short bit6 = a & 512; //  512 = 00000010 00000000

  if (bit6 == 512) {
    return (float) ( abs((int) rtv));
  }

  short bit7 = a & 256; // 256 = 00000001 00000000

  if (bit7 == 256) {
    return  ( -1.0 * rtv );
  }

  return rtv;
} // BlockProductDesc::getValue
