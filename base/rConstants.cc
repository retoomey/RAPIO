#include "rConstants.h"

using namespace rapio;

const SentinelDouble Constants::MissingData(-99900, 0.00001);

const SentinelDouble Constants::RangeFolded(-99901, 0.00001);

const SentinelDouble Constants::DataUnavailable(-99903, 0.00001);

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
