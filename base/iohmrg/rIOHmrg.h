#pragma once

#include "rIODataType.h"
#include "rIO.h"

#include <zlib.h>
#include <string.h> // errno strs

namespace rapio {
/* Exception wrapper for Errno gzip and  C function calls, etc. */
class ErrnoException : public IO, public std::exception {
public:
  ErrnoException(int err, const std::string& c) : std::exception(), retval(err),
    command(c)
  { }

  virtual ~ErrnoException() throw() { }

  int
  getErrnoVal() const
  {
    return (retval);
  }

  std::string
  getErrnoCommand() const
  {
    return (command);
  }

  std::string
  getErrnoStr() const
  {
    return (std::string(strerror(retval)) + " on '" + command + "'");
  }

private:

  /** The Errno value from the command (copied from errno) */
  int retval;

  /** The Errno command we wrapped */
  std::string command;
};

/** Simple gridded header for HMRG for parameter passing */
class IOHmrgGriddedHeader {
public:
  int year;               // !< 1-4 4-digit year of valid time in UTC
  int month;              // !< 5-8 month of valid time in UTC
  int day;                // !< 9-12 day of valid time in UTC
  int hour;               // !< 13-16 hour of valid time in UTC
  int min;                // !< 17-20 minute of valid time in UTC
  int sec;                // !< 21-24 second of valid time in UTC
  int num_x;              // !< 25-28 number of data columns (NX)
  int num_y;              // !< 29-32 number of data columns (NY)
  int num_z;              // !< 33-36 number of data vertical levels (NZ); = 1 for 2D data
  std::string projection; // 37-40 map projection (="LL  " always)
  int map_scale;          // 41-44
  // scaled ints must be floats after division
  float lat1;   // 45-48
  float lat2;   // 49-52
  float lon;    // 53-56
  float lonnw;  // 57-60
  float center; // 61-64
  int depScale; // 65-68
  // scaled to dxy_scale
  float gridCellLonDegs; // 69-72
  float gridCellLatDegs; // 73-76
  float scaleCellSize;   // 77-80

  std::string varName; // variable name such as CREF1
  std::string varUnit; // variable unit such as dBZ
  int var_scale;
};

/**
 * Read HMRG binary files, an internal format we use in NSSL
 * Based on reader work done by others
 *
 * @author Jian Zhang
 * @author Carrie Langston
 * @author Robert Toomey
 */
class IOHmrg : public IODataType {
public:

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readRawDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  // Could move these to other files which might make it cleaner

  /** Do the heavy work of creating a RadialSet */
  static std::shared_ptr<DataType>
  readRadialSet(gzFile fp, const std::string& radarName, bool debug = false);

  /** Do the heavy work of creating a Gridded type */
  static std::shared_ptr<DataType>
  readGriddedType(gzFile fp, IOHmrgGriddedHeader& h, bool debug = false);

  /** Do the heavy work of creating a LatLonGrid */
  static std::shared_ptr<DataType>
  readLatLonGrid(gzFile fp, IOHmrgGriddedHeader& h, bool debug = false);

  /** Do the heavy work of creating a LatLonHeightGrid
   * No excuses now I need to add the 3D class soon */
  static std::shared_ptr<DataType>
  readLatLonHeightGrid(gzFile fp, IOHmrgGriddedHeader& h, bool debug = false);

  virtual
  ~IOHmrg();
};
}

// Code readablity...
#define ERRNO(e) \
  { { e; } if ((errno != 0)) { throw ErrnoException(errno, # e); } }
