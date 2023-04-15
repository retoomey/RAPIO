#include "rConstants.h"

#include <cmath> // for M_PI

using namespace rapio;

/** Default header for RAPIO */
const std::string Constants::RAPIOHeader = "Realtime Algorithm Parameter and IO (RAPIO) ";

// FIXME: Need to debate check/this.  MRMS was using multiple values everywhere,
// which is the 'best'?  Earth varies from 6357 at poles to 6378 equator km
// It's better to define a number instead of an object.  This is mostly used
// in very intensive math projection.  A define might even be better here
// double EarthRadius         = 6370949.0; Didn't match with us anyway..wow
// double EarthRadius         = 6371000.0; Used by all the old storm motion stuff
const double Constants::EarthRadiusM  = 6378000.0;
const double Constants::EarthRadiusKM = 6378.0;

const SentinelDouble Constants::MissingData(-99900, 0.00001);

const SentinelDouble Constants::RangeFolded(-99901, 0.00001);

const SentinelDouble Constants::DataUnavailable(-99903, 0.00001);

const double Constants::RadiansPerDegree = M_PI / 180.0;

const double Constants::DegreesPerRadian = 180.0 / M_PI;

const time_t Constants::SecondsPerDay = 86400;

// Datatype constants
const std::string Constants::ColorMap         = "ColorMap";
const std::string Constants::IsTableData      = "IsTableData";
const std::string Constants::ExpiryInterval   = "ExpiryInterval";
const std::string Constants::FilenameDateTime = "FilenameDateTime";

// Terrain constants
const std::string Constants::TerrainBeamBottomHit = "TerrainBeamBottomHit";
const std::string Constants::TerrainPBBPercent    = "TerrainPBBPercent";
const std::string Constants::TerrainCBBPercent    = "TerrainCBBPercent";

// Datatype reading/writing
const std::string Constants::PrimaryDataName("primary");
const std::string Constants::TypeName("TypeName");
const std::string Constants::sDataType("DataType");
// Note: W2 was changed to write 'units' instead of 'Units'.  So we have
// archive cases with 'Units' but all new stuff is 'units'.
// We'll make readers look for both, but this string will write
const std::string Constants::Units("units");
const std::string Constants::Latitude("Latitude");
const std::string Constants::Longitude("Longitude");
const std::string Constants::Height("Height");
const std::string Constants::Time("Time");
const std::string Constants::FractionalTime("FractionalTime");

// Record
const std::string Constants::IndexPathReplace("{indexlocation}");
const std::string Constants::RecordXMLSeparator(" ");
