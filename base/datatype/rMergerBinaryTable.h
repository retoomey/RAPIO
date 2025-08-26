#pragma once

#include <rBinaryTable.h>
#include <rLLH.h>
#include <rTime.h>
#include <string>
#include <vector>

namespace rapio {
/** Binary tables used by w2merger.  These may eventually deprecate, but this allows
 * rdump and other tools to read them if needed.
 */
class WObsBinaryTable : public BinaryTable
{
public:

  /** Create a weighted observation binary table.  Think this is deprecated in MRMS */
  WObsBinaryTable()
  {
    // Lookup for read/write factories
    myDataType = "WObsBinaryTable";
  }

  /** Write our block to file at current location */
  virtual bool
  writeBlock(FILE * fp) override;

  /** Our block level...which is level 1 and the first always.  Every subclass
   * should increase their count by 1 (depth of subclass tree.  Siblings have same value) */
  static size_t BLOCK_LEVEL;

  /** Typename of the data such as Reflectivity */
  std::string typeName;

  /** Optional marked lines cache filename, blank if not used.
   * I'm wondering if this is ever used currently in MRMS. */
  std::string markedLinesCacheFile;

  /** Origin of the data: radar center or the nw corner of domain for example. */
  float lat, lon, ht;

  /** The global time of all the data */
  time_t data_time;

  /** The global valid time of all the data */
  time_t valid_time;

  // Arrays
  struct Line {
    unsigned short x, y, z, len;
  };
  std::vector<Line> markedLines;
  std::vector<unsigned short> x, y, z;
  std::vector<float> newvalue;
  std::vector<unsigned short> scaled_dist;
  std::vector<char> elevWeightScaled;

  /** Add an observation.  Subclasses should create new methods for extra fields and call this. */
  inline void
  addWeightedObservation(int i, int j, int k, float val,
    unsigned short scaled_range, int scaled_elev_wt)
  {
    // Add to each list of stored data...subclasses can add more...
    newvalue.push_back(val);
    scaled_dist.push_back(scaled_range);
    x.push_back(i);
    y.push_back(j);
    z.push_back(k);
    elevWeightScaled.push_back((char) (scaled_elev_wt));
  }

  /** Get the block level magic vector for this class.  Subclasses MUST override
   * to call their
   * superclass and then push back their level identifier. */
  virtual void
  getBlockLevels(std::vector<std::string>& levels) override;

  /** Read our block from file if it exists at current location */
  virtual bool
  readBlock(const std::string& path, FILE * fp) override;

  /** Send human readable output to a ostream.  This is
   * called by iotext and rdump to view the file */
  virtual bool
  dumpToText(std::ostream& s) override
  {
    const std::string i = "\t";

    s << i << "This is a WObs binary table.\n";
    return true;
  };
};

class RObsBinaryTable : public WObsBinaryTable
{
public:

  /** Create a weighted observation binary table.  Think this is deprecated in MRMS */
  RObsBinaryTable()
  {
    // Lookup for read/write factories
    myDataType = "RObsBinaryTable";
  }

  /** Write our block to file at current location */
  virtual bool
  writeBlock(FILE * fp) override;

  /** Our block level...which is level 1 and the first always.  Every subclass
   * should increase their count by 1 (depth of subclass tree.  Siblings have same value) */
  static size_t BLOCK_LEVEL;

  /** Radar name these observations belong to */
  std::string radarName;

  /** VCP number of the radar */
  int vcp;

  /** Elevation of the radar */
  float elev;

  /** Azimuth values */
  std::vector<unsigned short> azimuth;

  /** Ouch, MRMS using the code::Time here.  For a 'general' format
   * I'm gonna have to change that.  Try to make a structure with same size.
   * FIXME: modify WDSSII raw format to use standard variables
   */
  struct mrmstime {
    time_t epoch_sec;
    double frac_sec;
  };

  /** Azimuth values */
  std::vector<mrmstime> aztime;

  /** Get the block level magic vector for this class.  Subclasses MUST override
   * to call their
   * superclass and then push back their level identifier. */
  virtual void
  getBlockLevels(std::vector<std::string>& levels) override;

  /** Add a raw observation. */
  inline void
  addRawObservation(int i, int j, int k, float val,
    unsigned short scaled_range, int scaled_elev_wt,
    unsigned short aAzimuth, mrmstime& aTime)
  {
    addWeightedObservation(i, j, k, val, scaled_range, scaled_elev_wt);
    azimuth.push_back(aAzimuth);
    aztime.push_back(aTime);
  }

  /** Read our block from file if it exists at current location */
  virtual bool
  readBlock(const std::string& path, FILE * fp) override;

  /** Send human readable output to a ostream.  This is
   * called by iotext and rdump to view the file */
  virtual bool
  dumpToText(std::ostream& s) override;
};
}
