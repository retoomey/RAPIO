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
  r.dump();

  // Product code and packet code
  int product     = d.getProductCode();
  const auto info = ConfigNIDSInfo::getNIDSInfo(product);

  // Check to see if this is a null product.
  if (info.isNullProduct()) {
    // FIXME: handle null product.
    fLogSevere("Null product not handled, product number: {}", product);
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

        // -------
        // Test the reverse function.  We already have the decoded thresholds
        //   std::vector<float> mvalues;
        std::vector<int> colors2; // write to
        const float * p = mvalues.data();
        NIDSUtil::valueToColorD3(decoded, p, mvalues.size(), colors2);

        for (size_t i = 0; i < colors2.size(); i++) {
          if (color_codes[i] != colors2[i]) {
            fLogSevere("Mismatch at {} G: {} != {}", i, color_codes[i], colors2[i]);
          }
        }
        // -------
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

  fLogInfo("Calculated elevation angle is {}", elevAngleDegs);
  fLogInfo("Calculated LLH is {}", location);
  fLogInfo("Calculated time is {}", time.getString());
  fLogInfo("Gatewidth is {} Meters", gate_width);
  fLogInfo("First Range is {} Meters", firstGateDistanceMeters);
  fLogInfo("Units is {}", units);
  fLogInfo("Name is {}", aTypename);

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
  // Use time from radialset
  short julianDate;
  int seconds;

  NIDSUtil::getNIDSTimeFromTime(rs.getTime(), julianDate, seconds);

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
  // header.myMsgLength = Later.  Symbology will vary.
  header.myMsgSrcID  = 0; // What to be?
  header.myMsgDstID  = 0; // What to be?
  header.myNumBlocks = 3; // Header, description, symbology

  BlockProductDesc d;

  d.setLocation(rs.getCenterLocation());
  setElevationAngleDegs(d, rs.getElevationDegs());
  d.setProductCode(MESSAGE_CODE);
  d.setOpMode(2); // no idea.
  d.setVCP(rs.getVCP());
  d.setSeqNum(0);     // no idea.
  d.setVolScanNum(0); // no idea
  d.setVolumeStartTime(rs.getTime());
  d.setGenTime(rs.getTime());

  // d.myDeps[0-9] = 0;
  d.setElevationNum(0);
  // d.myDataThresholds[0-15]
  d.setNumberMaps(0);       // 256 on our data but eh no idea
  d.mySymbologyOffset = 60; // FIXME: Not sure how to calculate this.
                            // None of the sizes seem to match up
  d.myGraphicOffset = 0;
  d.myTabularOffset = 0;

  BlockProductSymbology sym;

  sym.myBlockID     = 0;
  sym.myBlockLength = 0;
  sym.myNumLayers   = 1; // one I guess?

  // Gotta prep the BlockRadialSet for writing.
  BlockRadialSet r;

  r.myPacketCode     = 16;          ///< Packet Type x'AF1F' or 16 or 28
  r.myIndexFirstBin  = 0;           ///< Index of first range bin
  r.myNumRangeBin    = num_gates;   ///< Number of range bins comprising a radial
  r.myCenterOfSweepI = 0;           ///< I coordinate of center of sweep
  r.myCenterOfSweepJ = 0;           ///< J coordinate of center of sweep
  r.myScaleFactor    = 999;         ///< Number of pixels per range bin.
  r.myNumRadials     = num_radials; ///< Total number of radials in products

  ArrayFloat2DPtr myOutputArray = rs.getFloat2DPtr();
  ArrayFloat1DPtr bwDegs        = rs.getFloat1DPtr(RadialSet::BeamWidth);
  ArrayFloat1DPtr azDegs        = rs.getFloat1DPtr(RadialSet::Azimuth);
  ArrayFloat1DPtr gwMeters      = rs.getFloat1DPtr(RadialSet::GateWidth);

  // Gate width is stored in info, there's no field in NIDS it seems.
  // We can still check to see if we match though
  const LengthMs gate_width = info.getGateWidthMeters();
  bool mismatch = false;
  int first     = -1;

  for (size_t radial = 0; radial < num_radials; radial++) {
    if ((*gwMeters)[radial] != gate_width) {
      mismatch = true;
      first    = radial;
    }
  }
  if (mismatch) {
    fLogSevere("Mismatch between gate width in info table {} and netcdf gatewidth {}",
      gate_width, (*gwMeters)[first]);
  }

  // First need to calculate the thresholds and set the product block.
  std::vector<float> thresholds;

  // There are ultimately only 16 values, just super overloaded in meaning.

  // Starting with the decode method2, since my example file uses it.
  // FIXME: will have to add all the other methods, but I'll have to look
  // at more file examples to tweak.  RAPIO mostly just needs to read level III,
  // not write it.  But I'm ok adding some ability as needed for special cases.
  // This was product 159.  We'd need to store these probably in xml.
  // This will make the threadhold table work.
  d.setDecodeMethod2(16, 128, 2, 255); // scale, offset, leading, numvalues
  // Now we can create the thresholds lookup table.
  d.decodeMethod2(thresholds);

  // mrms values to colors.  We'll do the entire data set as a whole.
  // Note colors are indexes into the true values (thresholds) similiar
  // to space saving techniques of color indexed ancient video cards.
  std::vector<int> colors; // write to this
  const float * p = myOutputArray->data();

  NIDSUtil::valueToColorD3(thresholds, p, num_gates * num_radials, colors);

  // Direct color write.  FIXME: we have the different color modes
  size_t colori = 0;

  for (size_t radial = 0; radial < num_radials; radial++) {
    BlockRadialSet::RadialData radialLayer;
    radialLayer.start_angle = (*azDegs)[radial];
    radialLayer.delta_angle = (*bwDegs)[radial];
    radialLayer.inShorts    = true;

    for (size_t gate = 0; gate < num_gates; gate++) {
      radialLayer.data.push_back(colors[colori++]);
    }
    r.myRadials.push_back(radialLayer);
  }
  const bool compress = info.getChkCompression();

  // If compressed, first write the final stuff into a temp
  // storage and compress it,
  // so we can set the header length properly. Otherwise,
  // we'll hold off writing.
  std::vector<char> * compressedData = nullptr;
  size_t endSize;
  // b.writeVector(compressedData, compressedData->size());
  MemoryStreamBuffer bz;

  // Since we'll just compress to here, endian 'probably'
  // doesn't matter in this case.
  bz.setDataBigEndian();

  if (compress) {
    fLogInfo("Compression is ON");
    // Temp to write and compress
    MemoryStreamBuffer temp;
    temp.setDataBigEndian(); // NIDS big endian
    sym.write(temp);
    r.write(temp);
    bz = temp.writeBZIP2();

    compressedData = bz.getData();
    endSize        = compressedData->size(); // Final compressed size
  } else {
    fLogInfo("Compression is OFF");
    endSize = sym.size() + r.size();
  }
  header.myMsgLength = header.size() + d.size() + endSize;

  // Finally write everything...
  header.write(b); // Block 1
  d.write(b);      // Block 2
  if (compress) {
    b.writeVector(compressedData->data(), endSize);
    // Better be '66' or 'B' for bzip2
    // for (size_t i = 0; i < 4; i++) {
    //  std::cout << std::hex << (int) ((*compressedData)[i]) << ", ";
    // }
    // std::cout << "\n";
  } else {
    sym.write(b); // Block 3
    r.write(b);   // Layers part of sym block it seems
  }

  return true;
} // NIDSRadialSet::writeNIDS
