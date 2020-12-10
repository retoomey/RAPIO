#include "rConstants.h"

#include "rLength.h"

#include <cmath> // for M_PI

using namespace rapio;

/** Default header for RAPIO */
const std::string Constants::RAPIOHeader = "Realtime Algorithm Parameter and IO (RAPIO) ";

const Length&
Constants::EarthRadius()
{
  // double EarthRadius         = 6370949.0; Didn't match with us anyway..wow
  static Length result = Length::Meters(6.380E+06);

  return (result);
}

const SentinelDouble Constants::MissingData(-99900, 0.00001);

const SentinelDouble Constants::RangeFolded(-99901, 0.00001);

const SentinelDouble Constants::DataUnavailable(-99903, 0.00001);

const double Constants::RadiansPerDegree = M_PI / 180.0;

const double Constants::DegreesPerRadian = 180.0 / M_PI;

const time_t Constants::SecondsPerDay = 86400;

// Datatype constants
const std::string Constants::ColorMap         = "ColorMap";
const std::string Constants::IsTableData      = "IsTableData";
const std::string Constants::Unit             = "Unit";
const std::string Constants::ExpiryInterval   = "ExpiryInterval";
const std::string Constants::FilenameDateTime = "FilenameDateTime";


// Datatype reading/writing
const std::string Constants::TypeName("TypeName");
const std::string Constants::sDataType("DataType");
const std::string Constants::Units("Units");
const std::string Constants::Latitude("Latitude");
const std::string Constants::Longitude("Longitude");
const std::string Constants::Height("Height");
const std::string Constants::Time("Time");
const std::string Constants::FractionalTime("FractionalTime");

// Record
const std::string Constants::IndexPathReplace("{indexlocation}");
const std::string Constants::RecordXMLSeparator(" ");
