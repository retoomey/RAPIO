#pragma once

#include <rUtility.h>

#include <cmath> // for fabs
#include <ctime> // for time_t
#include <string>

#include <rSentinelDouble.h>

#ifndef M_PI
// Source: http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html
# define M_PI 3.141592653589793238462643383279502884197169399375105820974944592307816406
#endif

// Convenient shortcuts
#define DEG_TO_RAD (Constants::RadiansPerDegree)
#define RAD_TO_DEG (Constants::DegreesPerRadian)

// Still debating unit classes (the slowness of unit safety vs the speed of labeling units in variables)
// The issue with classes comes when we have millions/billions of angles, lengths, etc.
// FIXME: Actually...I think we should have both ways and link them together.
#define AngleDegs float
#define LengthKMs float
#define LengthMs  float

namespace rapio {
/** Hold all constants for RAPIO
 *
 * Using non caps for constants since c++ standard recommends not to do
 * this in order to avoid conflicts with compiler macros.  All caps is left
 * over from old days.
 *
 * @author Robert Toomey */
class Constants : public Utility
{
public:
  /** Default header for RAPIO */
  static constexpr const char * RAPIOHeader = "Realtime Algorithm Parameter and IO (RAPIO) ";

  // GIS constants

  // FIXME: Need to debate check/this.  MRMS was using multiple values everywhere,
  // which is the 'best'?  Earth varies from 6357 at poles to 6378 equator km
  // It's better to define a number instead of an object.  This is mostly used
  // in very intensive math projection.  A define might even be better here
  // double EarthRadius         = 6370949.0; Didn't match with us anyway..wow
  // double EarthRadius         = 6371000.0; Used by all the old storm motion stuff

  /** The mean equatorial radius of the earth in meters. */
  static constexpr double EarthRadiusM = 6378000.0;

  /** The mean equatorial radius of the earth in kilometers. */
  static constexpr double EarthRadiusKM = 6378.0;

  /** Radians per degree. */
  static constexpr double RadiansPerDegree = M_PI / 180.0;

  /** Degrees per radian. */
  static constexpr double DegreesPerRadian = 180.0 / M_PI;

  /** The number of seconds in one day. */
  static constexpr time_t SecondsPerDay = 86400;

  // Datatype constants
  static constexpr const char * ColorMap         = "ColorMap";
  static constexpr const char * IsTableData      = "IsTableData";
  static constexpr const char * ExpiryInterval   = "ExpiryInterval";
  static constexpr const char * FilenameDateTime = "FilenameDateTime";

  // Terrain constants
  static constexpr const char * TerrainBeamBottomHit = "TerrainBeamBottomHit";
  static constexpr const char * TerrainPBBPercent    = "TerrainPBBPercent";
  static constexpr const char * TerrainCBBPercent    = "TerrainCBBPercent";

  // Constants for reading/writing attributes

  /** Used to read first/primary layer of DataTypes */
  static constexpr const char * PrimaryDataName = "primary";
  /** What is the TypeName is present in this file? */
  static constexpr const char * TypeName = "TypeName";

  /** What is the DataType present in this file, i.e. the name of the
   *  descendant of DataType? e.g. RadialSet, PointSetData, etc. */
  static constexpr const char * sDataType = "DataType";

  // Note: W2 was changed to write 'units' instead of 'Units'.  So we have
  // archive cases with 'Units' but all new stuff is 'units'.
  static constexpr const char * Units          = "units";
  static constexpr const char * Latitude       = "Latitude";
  static constexpr const char * Longitude      = "Longitude";
  static constexpr const char * Height         = "Height";
  static constexpr const char * Time           = "Time";
  static constexpr const char * FractionalTime = "FractionalTime";

  /** Parameter to replace in fam files, etc for relative index location */
  static constexpr const char * IndexPathReplace = "{indexlocation}";

  /** Used to separate xml parameters for fam, etc. */
  static constexpr const char * RecordXMLSeparator = " ";

  /** The value that is assigned to data that is missing. */
  static const SentinelDouble MissingData;

  /** The value that is assigned to range-folded doppler
   * velocity data regardless of the source of the data
   * (hires/orpg/awips, etc.).
   */
  static const SentinelDouble RangeFolded;

  /** Data not available. MissingData is typically used when
   *  data is available, but below threshold */
  static const SentinelDouble DataUnavailable;

  /** Returns true if the value is not one of Missing,
   *  RangeFolded or DataUnavailable. */
  inline static bool
  isGood(double val)
  {
    // check the most general case first, then specials
    return (val > -99899) || (val != MissingData && val != DataUnavailable && val != RangeFolded);
  }

  /** A quick and dirty version of isGood. This won't work if you
   *  will have valid values below -99000 */
  inline static bool
  isGood_approx(double val)
  {
    return (val > -99899);
  }
};

//  end struct Constants
}

//  end namespace rapio
