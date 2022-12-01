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
  static const std::string RAPIOHeader;

  // GIS constants

  /** The mean equatorial radius of the earth in meters. */
  static const double EarthRadiusM;

  /** The mean equatorial radius of the earth in kilometers. */
  static const double EarthRadiusKM;

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
    // check the most general case first
    if (val > -99899) { return (true); }

    return (val != MissingData && val != DataUnavailable && val != RangeFolded);
  }

  /** A quick and dirty version of isGood. This won't work if you
   *  will have valid values below -99000 */
  inline static bool
  isGood_approx(double val)
  {
    return (val > -99899);
  }

  /** Radians per degree. */
  static const double RadiansPerDegree;

  /** Degrees per radian. */
  static const double DegreesPerRadian;

  /** The number of seconds in one day. */
  static const time_t SecondsPerDay;

  // DataType constants
  static const std::string ColorMap;
  static const std::string IsTableData;
  static const std::string Unit;
  static const std::string ExpiryInterval;
  static const std::string FilenameDateTime; // Formatted as filename expected

  // Terrain constants
  static const std::string TerrainBeamBottomHit;
  static const std::string TerrainPBBPercent;
  static const std::string TerrainCBBPercent;

  // Constants for reading/writing attributes

  /** Used to read first/primary layer of DataTypes */
  static const std::string PrimaryDataName;

  /** What is the TypeName is present in this file? */
  static const std::string TypeName;

  /** What is the DataType present in this file, i.e. the name of the
   *  descendant of DataType? e.g. RadialSet, PointSetData, etc. */
  static const std::string sDataType;

  static const std::string Units;
  static const std::string Latitude;
  static const std::string Longitude;
  static const std::string Height;
  static const std::string Time;
  static const std::string FractionalTime;

  /** Parameter to replace in fam files, etc for relative index location */
  static const std::string IndexPathReplace;

  /** Used to separate xml parameters for fam, etc. */
  static const std::string RecordXMLSeparator;
};

//  end struct Constants
}

//  end namespace rapio
