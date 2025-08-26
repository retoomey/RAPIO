#pragma once

#include <rIO.h>
#include <rDataType.h>
#include <rDataGrid.h>
#include <rLatLonGrid.h>

namespace rapio {
/** A field of a grib2 message.
 * Note:  Noticed that here we use virtual interface and store actual values in the gribfield*
 * in the class, vs the message where we copy the section numbers.  Not sure if it matters really,
 * but might refactor at some point. */
class GribField : public Data {
public:

  /** Create uninitialized field */
  GribField(size_t messageNumber, size_t fieldNumber) : myMessageNumber(messageNumber), myFieldNumber(fieldNumber){ };

  // Array methods (assuming grid data)

  /** Get a float 2D array in data projection, assuming we're grid data */
  virtual std::shared_ptr<Array<float, 2> >
  getFloat2D() = 0;

  /** Append/create a float 3D at a given level.  Used for 3D array ability */
  virtual std::shared_ptr<Array<float, 3> >
  getFloat3D(std::shared_ptr<Array<float, 3> > in, size_t at, size_t max) = 0;

  /** GRIB Edition Number (currently 2) 'gfld->version' */
  virtual long
  getGRIBEditionNumber() = 0;

  /** Discipline-GRIB Master Table Number (see Code Table 0.0) 'gfld->discipline' */
  virtual long
  getDisciplineNumber() = 0;

  /** Get the Significance of Reference Time (see Code Table 1.2) 'gfld->idsect[4]' */
  virtual size_t
  getSigOfRefTime() = 0;

  /** Get the time stored in this field. (usually just matches message time) 'glfd->idsect[5-10]' */
  virtual Time
  getTime() = 0;

  /** Return the Grid Definition Template Number (Code Table 3.1) */
  virtual size_t
  getGridDefTemplateNumber() = 0;

  /** Get a date string similar to wgrib2 d=time dump */
  std::string
  getDateString(const std::string& pattern = "%Y%m%d%H")
  {
    return (getTime().getString(pattern));
  }

  /** Get message number we belong to */
  size_t getMessageNumber(){ return myMessageNumber; }

  /** Get field number */
  size_t getFieldNumber(){ return myFieldNumber; }

  // Database lookup names

  /** Get product name from database */
  virtual std::string
  getProductName() = 0;

  /** Get level name from database */
  virtual std::string
  getLevelName() = 0;

  /** Matches given product/level */
  bool
  matches(const std::string& product, const std::string& level)
  {
    return ( (getProductName() == product) && (getLevelName() == level));
  }

  /** Print this field.  Called by base operator << virtually */
  virtual std::ostream& print(std::ostream& os){ return os; }

protected:

  size_t myMessageNumber; ///< Message we're part of (along with other possible fields)

  size_t myFieldNumber; ///< Field number of the message that we are
};

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
  size_t
  getMessageNumber() const { return myMessageNumber; }

  /** Get the offset location into the read source where we were */
  size_t
  getFileOffset() const { return myFileLocationAt; }

  /** Get the number of fields stored in the message */
  size_t
  getNumberFields() const { return myNumberFields; }

  // Section 1
  // FIXME: Probably should enum some things at some point

  /** Discipline-GRIB Master Table Number (see Code Table 0.0) */
  long
  getDisciplineNumber() const { return mySection0[0]; }

  /** GRIB Edition Number (currently 2) */
  long
  getGRIBEditionNumber() const { return mySection0[1]; }

  /** Length of GRIB message */
  long
  getMessageLength() const { return mySection0[2]; }

  /** Get Id of originating centre (Common Code Table C-1) */
  long
  getCenterID() const { return mySection1[0]; }

  /** Get Id of originating sub-centre (local table) */
  long
  getSubCenterID() const { return mySection1[1]; }

  /** Get GRIB Master Tables Version Number (Code Table 1.0) */
  long
  getMasterTableVersion() const { return mySection1[2]; }

  /** Get GRIB Local Tables Version Number */
  long
  getLocalTableVersion() const { return mySection1[3]; }

  /** Get Significance of Reference Time (Code Table 1.1) */
  long
  getTimeType() const { return mySection1[4]; }

  // mySection1[5] to mySection[10] is a Time

  /** Get the time stored in this message and all its fields */
  Time
  getTime() const;

  /** Get a date string similar to wgrib2 d=time dump */
  std::string
  getDateString(const std::string& pattern = "%Y%m%d%H") const;

  /** Get Production status of data (Code Table 1.2) */
  long
  getProductionStatus() const { return mySection1[11]; }

  /** Get Type of processed data (Code Table 1.3) */
  long
  getType() const { return mySection1[12]; }

  /** Get a new field from us.  This doesn't load anything */
  virtual std::shared_ptr<GribField> getField(size_t fieldNumber){ return nullptr; }

  /** Print this message and N fields.  Called by base operator << virtually */
  virtual std::ostream& print(std::ostream& os){ return os; }

  // ------------------------------------------------------
  // Low level access

  /** Attempt to read g2 information at a file location.  Used at low level scanning.
   * to initialize all of our class variables. */
  virtual bool
  readG2Info(size_t messageNum, size_t fileOffsetat)
  {
    return false;
  };

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

/** Root class of an action taken on each message of a Grib2 file.
 * Basically a scan is done where a GribAction gets to handle
 * each GribMessage of a file. No unpacking/expanding of data
 * is done until requested, which makes the scan fairly quick.
 *
 * @author Robert Toomey */
class GribAction : public IO
{
public:
  virtual bool
  action(std::shared_ptr<GribMessage>&, size_t fieldNumber)
  {
    return false; // Keep going?
  }
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
  getFloat2D(const std::string& key, const std::string& levelstr, const std::string& subtypestr = "" ) = 0;

  /** Get a 3D array from grib data */
  virtual std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec) = 0;

  /** Get a projected LatLonGrid from the grib data */
  virtual std::shared_ptr<LatLonGrid>
  getLatLonGrid(const std::string& key, const std::string& levelstr) = 0;
};

/** Output a GribMessage */
std::ostream&
operator << (std::ostream&,
  rapio::GribMessage&);
}
