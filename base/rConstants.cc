#include "rConstants.h"

using namespace rapio;

const SentinelDouble Constants::MissingData(-99900, 0.00001);

const SentinelDouble Constants::RangeFolded(-99901, 0.00001);

const SentinelDouble Constants::DataUnavailable(-99903, 0.00001);

// Clang wants a place for the constexpr to be stricter to c++,
// G++ lets it slide.  We'll be strict

// Physical constants
constexpr double Constants::EarthRadiusM;
constexpr double Constants::EarthRadiusKM;
constexpr double Constants::RadiansPerDegree;
constexpr double Constants::DegreesPerRadian;
constexpr time_t Constants::SecondsPerDay;

// Datatype constants
constexpr const char * Constants::ColorMap;
constexpr const char * Constants::IsTableData;
constexpr const char * Constants::ExpiryInterval;
constexpr const char * Constants::FilenameDateTime;

// Terrain constants
constexpr const char * Constants::TerrainBeamBottomHit;
constexpr const char * Constants::TerrainPBBPercent;
constexpr const char * Constants::TerrainCBBPercent;

// Data attribute constants
constexpr const char * Constants::PrimaryDataName;
constexpr const char * Constants::TypeName;
constexpr const char * Constants::sDataType;
constexpr const char * Constants::Units;
constexpr const char * Constants::Latitude;
constexpr const char * Constants::Longitude;
constexpr const char * Constants::Height;
constexpr const char * Constants::Time;
constexpr const char * Constants::FractionalTime;

// Indexing / replacement
constexpr const char * Constants::IndexPathReplace;
constexpr const char * Constants::RecordXMLSeparator;

// Datatype reading/writing
// const std::string Constants::TypeName("TypeName");
// const std::string Constants::sDataType("DataType");
// Note: W2 was changed to write 'units' instead of 'Units'.  So we have
// archive cases with 'Units' but all new stuff is 'units'.
// We'll make readers look for both, but this string will write
// const std::string Constants::Units("units");
// const std::string Constants::Latitude("Latitude");
// const std::string Constants::Longitude("Longitude");
// const std::string Constants::Height("Height");
// const std::string Constants::Time("Time");
// const std::string Constants::FractionalTime("FractionalTime");

// Record
// const std::string Constants::IndexPathReplace("{indexlocation}");
// const std::string Constants::RecordXMLSeparator(" ");
