#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

namespace rapio {
/** GribMessage interface, holding 1 or more fields.
 * It seems in practice the number of fields is 'usually' 1
 * Holder for grib2 field information and buffer storage information
 * that can refer to a global buffer and/or a local message buffer. */
class GribMessage : public Data {
public:

  /** Create uninitialized message */
  GribMessage(){ };

  //
  // Interface for querying message header
  //

  // Section 0

  /** Get the message number in the read source we represent */
  size_t getMessageNumber(){ return myMessageNumber; }

  /** Get the offset location into the read source where we were */
  size_t getFileOffset(){ return myFileLocationAt; }

  /** Get the number of fields stored in the message */
  size_t getNumberFields(){ return myNumberFields; }

  // Section 1
  // FIXME: Probably should enum some things at some point

  /** Discipline-GRIB Master Table Number (see Code Table 0.0) */
  long getDisciplineNumber(){ return mySection0[0]; }

  /** GRIB Edition Number (currently 2) */
  long getGRIBEditionNumber(){ return mySection0[1]; }

  /** Length of GRIB message */
  long getMessageLength(){ return mySection0[2]; }

  /** Get Id of originating centre (Common Code Table C-1) */
  long getCenterID(){ return mySection1[0]; }

  /** Get Id of originating sub-centre (local table) */
  long getSubCenterID(){ return mySection1[1]; }

  /** Get GRIB Master Tables Version Number (Code Table 1.0) */
  long getMasterTableVersion(){ return mySection1[2]; }

  /** Get GRIB Local Tables Version Number */
  long getLocalTableVersion(){ return mySection1[3]; }

  /** Get Significance of Reference Time (Code Table 1.1) */
  long getTimeType(){ return mySection1[4]; }

  // mySection1[5] to mySection[10] is a Time

  /** Get the time stored in this message and all its fields */
  Time
  getTime();

  /** Get a date string similar to wgrib2 d=time dump */
  std::string
  getDateString(const std::string& pattern = "%Y%m%d%H");

  /** Get Production status of data (Code Table 1.2) */
  long getProductionStatus(){ return mySection1[11]; }

  /** Get Type of processed data (Code Table 1.3) */
  long getType(){ return mySection1[12]; }

protected:
  /** Current message number in Grib2 data */
  size_t myMessageNumber;

  /** Current location in source file */
  size_t myFileLocationAt;

  /** Number of fields in message */
  size_t myNumberFields;

  /** Current Section 0 fields */
  long mySection0[3];

  /** Current Section 1 fields */
  long mySection1[13];

  /** Current number of local use sections in message. */
  size_t myNumberLocalUse;
};

/** DataType for holding Grib data
 * Interface for implementations of grib to
 * implement
 *
 * @author Robert Toomey */
class GribDataType : public DataType {
public:
  GribDataType()
  {
    myDataType = "GribData";
  }

  /** Print the catalog for thie GribDataType. */
  virtual void
  printCatalog() = 0;

  /** Get a message given key and level */
  virtual std::shared_ptr<GribMessage>
  getMessage(const std::string& key, const std::string& levelstr) = 0;

  /** One way to get 2D data, using key and level string like our HMET library */
  virtual std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr) = 0;

  /** Get a 3D array from grib data */
  virtual std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec) = 0;
};
}
