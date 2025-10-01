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
    LogSevere("No creating record available, can't create fake data.\n");
    return nullptr;
  }
  bool haveRadar = ConfigRadarInfo::haveRadar(params);

  if (!haveRadar) {
    LogSevere("No radar named '" << params << "', can't create fake data.\n");
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
  auto d = dt->getFloat2D();

  // How to fill data?  For start, vcp 212 we will fill the 4.0 angle
  if (dt->getElevationDegs() == 4.0) {
    d->fill(20);
  } else {
    d->fill(Constants::MissingData);
  }
  return dt;
} // IOFakeDataType::createDataType

bool
IOFakeDataType::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>                     & keys
)
{
  LogSevere("Unable to write fake data directly.\n");
  return false;
}
