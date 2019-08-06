#pragma once

#include <rUtility.h>

#include <cmath> // for fabs
#include <ctime> // for time_t
#include <string>

#include <rSentinelDouble.h>

namespace rapio {
class Length;

/** Hold all constants for RAPIO */
class Constants : public Utility
{
public:
  /** Default header for RAPIO */
  static const std::string RAPIOHeader;

  /** Type of Reader, Writer that uses flat files */
  static const std::string FlatFile;

  /** Type of Reader, Writer that uses .gz files */
  static const std::string GzippedFile;

  /** Type of Reader,        that uses .bz2 files */
  static const std::string BZippedFile;

  /** The average size of an XML message to be put in a Linear
   * Buffer.  The value 256 is the old avg size.  The Virtual Vol
   * product has increased this value to roughly 600.  */
  static const int AVERAGE_MSG_SIZE;

  /** The mean equatorial radius of the earth. */
  static const Length&
  EarthRadius();

  /** The value that is assigned to data that is missing.
   */
  static const SentinelDouble MISSING_DATA;

  /** The value that is assigned to data that is missing. */
  static const SentinelDouble MissingData;

  /** The value that is assigned to range-folded doppler velocity
   *  data regardless of the source of the data (hires/orpg/awips,
   *  etc.).
   */
  static const SentinelDouble RANGE_FOLDED;

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
  static const double RAD_PER_DEGREE;

  /** Radians per degree. */
  static const double RadiansPerDegree;

  /** Degrees per radian. */
  static const double DEGREE_PER_RAD;

  /** Degrees per radian. */
  static const double DegreesPerRadian;

  /** The angle corresponding to pi radians. */
  static const double&
  Pi();

  /** The number of seconds in one day. */
  static const time_t SecondsPerDay;

  // DataType constants
  static const std::string ColorMap;
  static const std::string IsTableData;
  static const std::string Unit;
  static const std::string ExpiryInterval;
  static const std::string FilenameDateTime; // Formatted as filename expected

  // Contants for reading/writing attributes

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
