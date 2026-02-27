#include "rIOFakeDataGenerator.h"

#include "rConfigRadarInfo.h"
#include "rRadialSet.h"
#include <rFactory.h>

using namespace rapio;

Record
RadarGenerator::generateRecord(const std::string& params, const Time& time)
{
  // Params is just radar name right now, like KTLX
  std::vector<std::string> recParams = { "fake", params };
  auto r = Record(recParams, "fake", time, "Reflectivity", VCP212_Elevs[myAtElevation]);

  if (++myAtElevation >= VCP212_Elevs.size()) { myAtElevation = 0; }
  return r;
}

std::shared_ptr<DataType>
RadarGenerator::createDataType(const Record& rec)
{
  // Move construction logic here from rIOFakeDataType.cc
  std::string radarName = rec.getParams()[1];

  bool haveRadar = ConfigRadarInfo::haveRadar(radarName);

  if (!haveRadar) {
    fLogSevere("No radar named '{}', can't create fake data.", radarName);
    nullptr;
  }

  LengthKMs firstGateDistanceKMs = 0.0;
  LengthKMs gateWidthMeters      = 250;
  size_t numRadials = 360;
  size_t numGates   = 100;

  LLH myCenter = ConfigRadarInfo::getLocation(radarName);

  AngleDegs elevDegs = std::stof(rec.getSubType());

  auto dt = RadialSet::Create("Reflectivity", "dbZ", myCenter, rec.getTime(),
      elevDegs, firstGateDistanceKMs / 1000.0, gateWidthMeters,
      numRadials, numGates);

  dt->setRadarName(radarName);
  dt->setVCP(212);

  // Apply the "dummy data" fill based on angle
  float fill = getFillValueForAngle(elevDegs);

  fLogInfo("Angle {} filling with {}", dt->getElevationDegs(), fill);

  dt->getFloat2D()->fill(fill);

  return dt;
} // RadarGenerator::createDataType

float
RadarGenerator::getFillValueForAngle(float angle)
{
  int key = static_cast<int>(angle * 10.0 + 0.5);

  float fill = Constants::MissingData;

  switch (key) {
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
      case 195: fill = 50;
        break;
      default:
        fill = Constants::MissingData;
        break;
  }
  return fill;
} // RadarGenerator::getFillValueForAngle
