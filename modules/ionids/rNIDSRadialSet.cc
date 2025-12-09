#include "rNIDSRadialSet.h"

#include "rNIDSUtil.h"
#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include "rBlockRadialSet.h"
#include "rConfigNIDSInfo.h"

using namespace rapio;
using namespace std;

NIDSRadialSet::~NIDSRadialSet()
{ }

void
NIDSRadialSet::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSRadialSet>();

  // NOTE: We read in polar grids and turn them into full radialsets
  owner->introduce("RadialSet", io);
  owner->introduce("PolarGrid", io);
  owner->introduce("SparseRadialSet", io);
  owner->introduce("SparsePolarGrid", io);
}

AngleDegs
NIDSRadialSet::getElevationAngleDegs(
  BlockProductDesc & d)
{
  // Put Desc class or not?  Since it's only
  // for RadialSets we'll keep here.

  const auto a = d.getDep(3);

  #if 0
  AngleDegs elevAngleDegs;
  if (d.getProductCode() == 84) { // not scaled
    elevAngleDegs = a;
  } else {
    elevAngleDegs = a * 0.1;
  }
  return elevAngleDegs;

  #endif
  return (d.getProductCode() == 84) ? a : a *0.1;
}

void
NIDSRadialSet::setElevationAngleDegs(
  BlockProductDesc & d, AngleDegs elevAngleDegs)
{
  if (d.getProductCode() == 84) { // FIXME: info
    d.setDep(3, elevAngleDegs);   // not scaled
  } else {
    d.setDep(3, elevAngleDegs * 10); // scaled
  }
}

std::shared_ptr<DataType>
NIDSRadialSet::readNIDS(
  std::map<std::string, std::string>& keys,
  BlockMessageHeader                & h,
  BlockProductDesc                  & d,
  BlockProductSymbology             & s,
  StreamBuffer                      & z)
{
  BlockRadialSet r;

  r.read(z);

  // Product code and packet code
  int product     = d.getProductCode();
  const auto info = ConfigNIDSInfo::getNIDSInfo(product);

  // Check to see if this is a null product.
  if (info.isNullProduct()) {
    // FIXME: handle null product.
    LogSevere("Null product not handled, product number: " << product << ".\n");
    return nullptr;
  }

  // Grab the decoded thresholds if needed by product
  // a 'bit' messy here because encoded needs shorts not floats
  // so we can't be completely generic
  std::array<short, 16> encoded = d.getEncodedThresholds();
  int decode = info.getDecode();
  // const bool needsDecode = info.needsDecodedThresholds();
  const bool needsDecode = (decode > 0); // all over 0
  std::vector<float> decoded;

  if (needsDecode) {
    d.getDecodedThresholds(decoded);
  }

  const size_t num_radials = r.getNumRadials();
  int packet       = r.getPacketCode();
  size_t num_gates = 1;
  std::vector<std::vector<float> > values;

  for (int i = 0; i < num_radials; i++) {
    auto& data = r.myRadials[i].data;

    // ----------------------------------------------------------
    // 1. raw values get converted to 'color codes'
    // based on the packet code

    // I think that the RadialData creation we could just
    // do the conversion, maybe on the fly on ingest.
    // This could save memory.  We just need the mrms float values
    // at this point.  We have to 'pack' any radial that is
    // smaller than the max with missing probably.
    // Though if we ever want rdump to print out 'native' contents
    // we may want to keep a copy of the original source data.
    // Could have both structures and a flag? Bleh
    // Of course, to 'write' data we need to make inverse of
    // these functions
    // FIXME: Question is, does this form of color code switch
    // occur in other datatypes and do we store it in the info table?
    std::vector<int> color_codes;
    if ((packet != 16) && (packet != 28)) {
      NIDSUtil::getRLEColors(data, color_codes);
    } else {
      if (packet == 16) {
        NIDSUtil::getColors(data, color_codes);
      } else { // packet  176
        NIDSUtil::getGenericColors(data, color_codes);
      }
    }

    // ----------------------------------------------------------
    // 2. int values 'color codes' converted to floats 'mrms'
    // based on the product code
    std::vector<float> mvalues;
    mvalues.reserve(color_codes.size());

    // FIXME: All these product codes should be in the info table
    // instead of hardcoded.

    // No decode, just lookup table type right?
    if (needsDecode) { // !134, !135 !176
      // Breaking down the various decoded methods
      if (product == 164) {
        NIDSUtil::colorToValueD1(color_codes, decoded, mvalues);
      } else if ( ( product == 165) || ( product == 176) || ( product == 177)) {
        NIDSUtil::colorToValueD2(color_codes, decoded, mvalues);
      } else if ( (product == 94) || (product == 99) ||
        (product == 153) || (product == 154) || (product == 155) ||
        (product == 159) || (product == 161) || (product == 163) )
      {
        NIDSUtil::colorToValueD3(color_codes, decoded, mvalues);
      } else {
        // Default fallback
        NIDSUtil::colorToValueD4(color_codes, decoded, mvalues);
      }
    } else {
      // It's looking like only 134, 135 and 176 use direct encoded thresholds
      // Everything use does all the decoding math.
      if (product == 134) {
        NIDSUtil::colorToValueE1(color_codes, encoded, mvalues);
      } else if (product == 135) {
        NIDSUtil::colorToValueE2(color_codes, encoded, mvalues);
      } else if (product == 176) {
        NIDSUtil::colorToValueE3(color_codes, encoded, mvalues);
      }
    }

    // Get the max gates in all the radials since we just pad to 2D array
    if (mvalues.size() > num_gates) {
      num_gates = mvalues.size();
    }
    // Buffer for now
    values.push_back(mvalues);
  }

  // ----------------------------------------------------------
  // 3. RadialSet attributes
  //
  const LLH location = d.getLocation();
  const Time time    = d.getVolumeStartTime();
  const AngleDegs elevAngleDegs = getElevationAngleDegs(d);
  const LengthMs gate_width     = info.getGateWidthMeters();
  const std::string aTypename   = info.getProductName();
  const std::string units       = info.getUnits();
  const LengthMs firstGateDistanceMeters = r.getFirstRangeBinIndex() * gate_width;

  LogInfo("Calculated elevation angle is " << elevAngleDegs << "\n");
  LogInfo("Calculated LLH is " << location << "\n");
  LogInfo("Calculated time is " << time.getString() << "\n");
  LogInfo("Gatewidth is " << gate_width << " Meters\n");
  LogInfo("First Range is " << firstGateDistanceMeters << " Meters\n");
  LogInfo("Units is " << units << "\n");
  LogInfo("Name is " << aTypename << "\n");

  auto radialSetSP = RadialSet::Create(aTypename, units, location, time,
      elevAngleDegs, firstGateDistanceMeters, gate_width, num_radials, num_gates);

  // Nyquist vel? todo
  radialSetSP->setVCP(d.getVCP());
  auto& rs = *radialSetSP;

  // Copy and fill
  // FIXME: We could possibly write directly into the arrays, could be
  // an optimization.
  ArrayFloat2DPtr myOutputArray = rs.getFloat2DPtr();
  ArrayFloat1DPtr bwDegs        = rs.getFloat1DPtr(RadialSet::BeamWidth);
  ArrayFloat1DPtr azDegs        = rs.getFloat1DPtr(RadialSet::Azimuth);
  ArrayFloat1DPtr gwMeters      = rs.getFloat1DPtr(RadialSet::GateWidth);

  for (size_t radial = 0; radial < num_radials; radial++) {
    auto& radialLayer = r.myRadials[radial];
    auto& value       = values[radial];
    (*gwMeters)[radial] = gate_width;              // Gatewidth
    (*azDegs)[radial]   = radialLayer.start_angle; // Azimuth
    (*bwDegs)[radial]   = radialLayer.delta_angle; // Beamwidth
    for (size_t gate = 0; gate < num_gates; gate++) {
      if (radial < value.size()) {
        (*myOutputArray)[radial][gate] = value[gate];
      } else {
        (*myOutputArray)[radial][gate] = Constants::MissingData;
      }
    }
  }

  // Todo the table attribute stuff maybe at some point
  return radialSetSP;
} // NIDSRadialSet::readNIDS

bool
NIDSRadialSet::writeNIDS(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt,
  StreamBuffer                      & b)
{
  auto radialsetp = std::dynamic_pointer_cast<RadialSet>(dt);

  // First failing attempt at writing.  This code is broken.
  // Switching to reading so I can understand the format well enough
  // to write it. 8)

  bool success = false;
  auto& rs     = *radialsetp;

  // ------------------------------------------------------
  // Specials.  For the moment hardcoded for alpha
  // I'm assuming parameters or a look up table to go from MRMS
  // product type to NIDS.

  // const uint16_t MESSAGE_CODE = 134; // AzShear
  // const uint16_t MESSAGE_TYPE = 31; // Digital Radial
  // const float minValue = -0.01f;
  // const float scale = 0.0001f;
  // const uint8_t missingVal = 0;

  // const uint16_t MESSAGE_CODE = 19; // Base Reflectivity
  // const uint16_t MESSAGE_TYPE = 16; // Graphic radial data
  // const float minValue = -33.0f; // dBZ;
  // const float scale = 0.5f;
  // const uint8_t missingVal = 0;

  // Matching the test file
  const uint16_t MESSAGE_CODE = 159;
  const auto info = ConfigNIDSInfo::getNIDSInfo(MESSAGE_CODE);
  const uint16_t MESSAGE_TYPE = 16;     // Graphic radial data
  const float minValue        = -33.0f; // dBZ;
  const float scale        = 0.5f;
  const uint8_t missingVal = 0;
  short julianDate         = 19852;
  int seconds = 72129;
  // Maybe we just port the objects.  kill me

  // --- 1. EXTRACT METADATA FROM RadialSet ---
  const size_t num_radials         = rs.getNumRadials();
  const size_t num_gates           = rs.getNumGates();
  const double ELEVATION_ANGLE_DEG = rs.getElevationDegs();
  const double GATE_SIZE_KM        = rs.getGateWidthKMs();
  // getDistanceToFirstGateM() returns meters, Level III uses KM for metadata description.
  const double RANGE_TO_FIRST_GATE_KM = rs.getDistanceToFirstGateM() / 1000.0;

  // Recommendation is using json perhaps for more details.  For now, just simple
  const std::string PRODUCT_DESC  = "MRMS/RAPIO";
  const std::string RADIAL_HEADER = "RadialSet";

  // Maybe we create all the blocks, then write them afterwards,
  // this way we can have all the offsets correct
  // I think IONIDS will pass the default blocks.
  BlockMessageHeader header;

  header.myMsgCode = MESSAGE_CODE;
  header.myMsgDate = julianDate;
  header.myMsgTime = seconds;
  // header.myMsgLength = something;
  header.myMsgSrcID  = 0; // What to be?
  header.myMsgDstID  = 0; // What to be?
  header.myNumBlocks = 1; // Start with one

  BlockProductDesc d;

  d.setLocation(rs.getCenterLocation());
  setElevationAngleDegs(d, rs.getElevationDegs());
  d.setProductCode(MESSAGE_CODE);
  d.setOpMode(0); // no idea.  I think when we read we could store as attributes
  d.setVCP(rs.getVCP());
  d.setSeqNum(0);     // no idea.
  d.setVolScanNum(0); // no idea
  d.setVolumeStartTime(rs.getTime());
  d.setGenTime(rs.getTime());

  // d.myDeps[0-9] = 0;
  d.setElevationNum(0);
  // d.myDataThresholds[0-15]
  d.setNumberMaps(0);
  d.mySymbologyOffset = 0; // FIXME: set this right?
  d.myGraphicOffset   = 0;
  d.myTabularOffset   = 0;

  BlockProductSymbology sym;

  sym.myBlockID     = 0;
  sym.myBlockLength = 0;
  sym.myNumLayers   = 1; // one I guess?

  // Gotta prep the BlockRadialSet for writing.
  BlockRadialSet r;

  // r.read(z);

  // Write them?
  header.write(b);
  d.write(b);

  // FIXME: if compressed?
  // wait we have to fill it all first, then write, right?
  // writeBZIP(b);  // write compressed set, right?
  // Humm ok how do we 'turn on' bzip2 compression?
  sym.write(b);

  r.write(b);

  #if 0
  for (size_t r = 0; r < NUM_RADIALS; ++r) {
    // Guess it wants the info per radial here.
    // Per ICD, azimuths are stored as integer degrees x 10
    float azStartDeg = azDegs[r];
    float azDeltaDeg = bwDegs[r];
    b.writeShort(static_cast<int16_t>(azStartDeg * 10));
    b.writeShort(static_cast<int16_t>(azDeltaDeg * 10));
    // Ahh so they allow variable gate lengths.  We don't though.
    b.writeShort(static_cast<int16_t>(NUM_GATES));

    for (size_t g = 0; g < NUM_GATES; ++g) {
      auto const& v = data[r][g];

      // Handle missing value case
      if (v == -99999.0f) {
        b.writeChar((char) (missingVal));
        continue;
      }
      // Handle good data
      int level = static_cast<int>(std::round((v - minValue) / scale));
      if (level < 0) { level = 0; } else if (level > 255) { level = 255; }
      uint8_t byteVal = static_cast<uint8_t>(level);
      b.writeChar((char) (byteVal));
    }
  }
  #endif // if 0

  return true;
} // NIDSRadialSet::writeNIDS
