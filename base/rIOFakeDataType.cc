#include "rIOFakeDataType.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
// #include "rBOOST.h"

// Default built in DataType support
#include "rDataTable.h"

// Fake creation, first attempt
#include "rConfigRadarInfo.h"
#include "rRadialSet.h"

using namespace rapio;

std::string
IOFakeDataType::getHelpString(const std::string & key)
{
  std::string help = "Fake DataType builder (Used by fake index.  Read only)";

  return help;
}

void
IOFakeDataType::initialize()
{ }

/** Read call */
std::shared_ptr<DataType>
IOFakeDataType::createDataType(const std::string& params)
{
  // We're always created by a record.  Not every createDataType
  // is since the system can read files.  But we shouldn't have
  // any fake files reading directly.
  auto * r = Record::getCreatingRecord();

  if (r == nullptr) {
    fLogSevere("No creating record available, can't create fake data.");
    return nullptr;
  }

  // FIXME: I think we could do plugins/factory introduce based on the params.  Currently
  // params is just the radar name.  For example -i fake=KTLX
  bool haveRadar = ConfigRadarInfo::haveRadar(params);

  if (!haveRadar) {
    fLogSevere("No radar named '{}', can't create fake data.", params);
    nullptr;
  }

  LengthKMs firstGateDistanceKMs = 0.0;
  LengthKMs gateWidthMeters      = 250;
  size_t numRadials = 360;
  size_t numGates   = 100;

  // LLH myCenter(35.3331, -97.2778, 390.0/1000.0);
  LLH myCenter = ConfigRadarInfo::getLocation(params);

  // FIXME: handle error here
  AngleDegs elevDegs = std::stof(r->getSubType());

  auto dt = RadialSet::Create("Reflectivity", "dbZ", myCenter, r->getTime(),
      elevDegs, firstGateDistanceKMs / 1000.0, gateWidthMeters,
      numRadials, numGates);

  dt->setRadarName(params);
  dt->setVCP(212);
  const int angle = (dt->getElevationDegs() * 10.0) + 0.5; // 0.5 --> 5
  auto d = dt->getFloat2D();

  // std::vector<std::string> angleDegs = {
  //  "0.5", "0.9", "1.3", "1.8", "2.4", "3.1", "4.0", "5.1", "6.4", "8.0", "10.0", "12.5", "15.6", "19.5" };
  float fill = Constants::MissingData;

  switch (angle) {
      case 5: fill = 5;
        break;
      case 9: fill = Constants::MissingData;
        break;
      case 13: fill = Constants::MissingData;
        break;
      case 18: fill = Constants::MissingData;
        break;
      case 24: fill = Constants::MissingData;
        break;
      case 31: fill = 8;
        break;
      case 40: fill = 10;
        break;
      case 51: fill = 12;
        break;
      case 64: fill = Constants::MissingData;
        break;
      case 80: fill = 15;
        break;
      case 100: fill = Constants::MissingData;
        break;
      case 125: fill = Constants::MissingData;
        break;
      case 156: fill = Constants::MissingData;
        break;
      case 195: fill = 20;
        break;
      default:
        fill = Constants::MissingData;
        break;
  }
  fLogInfo("Angle {} filling with {}", dt->getElevationDegs(), fill);
  d->fill(fill);
  return dt;
} // IOFakeDataType::createDataType

bool
IOFakeDataType::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>                     & keys
)
{
  fLogSevere("Unable to write fake data directly.");
  return false;
}
