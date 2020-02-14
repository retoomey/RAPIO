#pragma once

#include <rDataType.h>
#include <rTime.h>

#include <memory>
#include <vector>
#include <iostream>

#include <boost/property_tree/ptree.hpp>

namespace rapio {
class IndexType;
class RecordFilter;
class RecordQueue;

/** Records store individual information for items in a index.
 * Typically a single DataType object can be created from a single Record. */
class Record : public Data {
public:

  /** Creates an empty record. */
  Record(){ }

  /** Create a record with given time, parameters and selections.  */
  Record(const std::vector<std::string>& params,
    const std::vector<std::string>     & selects,
    const rapio::Time                  & productTime);

  /**
   * Create the object referenced by this record.
   * If this record corresponds to a montage, specify the index
   * of the DataType you want.
   *
   * @return invalid-pointer on error.
   */
  std::shared_ptr<DataType>
  createObject(size_t i = 0) const;

  /** For a valid record, returns builder parameters.
   *  If this record corresponds to a montage, specify the index
   *  of the DataType you want.
   * This is the full set of parameters, suitable for passing
   * immediately to the Builder's getBuilderWithFullParams
   */
  const std::vector<std::string>&
  getBuilderParams(size_t i = 0) const
  {
    return (myParams);
  }

  /** For a valid record, returns selection criteria. */
  const std::vector<std::string>&
  getSelections() const
  {
    return (mySelections);
  }

  /** The time of the product referenced by this record. */
  const Time&
  getTime() const
  {
    return (myTime);
  }

  /** What is the data source for this record?  For example, netcdf */
  const std::string&
  getDataSourceType() const;

  /**
   * The second selection criterion is the data type for data records
   * or the event name (e.g: "NewElevation") for event records.
   */
  const std::string&
  getDataType() const;

  /**
   * The second selection criterion is the data type for data records
   * or the event name (e.g: "NewElevation") for event records.
   */
  const std::string&
  getEventType() const;

  /**
   *  Does this record match the specification? The specification
   *  may be of two forms:
   *  <ol>
   *     <li> DataType alone e.g:  Reflectivity
   *     <li> DataType:Subtype   e.g:  Reflectivity:00.50
   *  </ol>
   */
  bool
  matches(const std::string& spec) const;

  /** Sorting of records happens by time, i.e. in chronological order.  */
  friend bool
  operator < (const Record& a,
    const Record          & b);

  /** Does this record correspond to a montage or to only one
   *  DataType? */
  //  bool isMontage() const
  //  {
  //    return (myParams.size() > 1);
  //  }

  /** Does this record correspond to an event or to data? */
  bool
  isEvent() const
  {
    return (myParams.front() == "Event");
  }

  bool
  isValid() const
  {
    return (myParams.size() > 0);
  }

  /** Get the subtype of record from mySelections */
  bool
  getSubtype(std::string& str);

  /** The index number that made this record */
  size_t
  getIndexNumber() const { return myIndexCount; }

  /** Set the index number */
  void
  setIndexNumber(size_t i){ myIndexCount = i; }

  /** Read param tag of record. */
  virtual void
  readParams(const std::string & params,
    const std::string          & changes,
    std::vector<std::string>   & v,
    const std::string          & indexPath);

  /** Record can read contents of (typically) an <item> tag and fill itself */
  virtual bool
  readXML(const boost::property_tree::ptree& itemNode,
    const std::string                      &indexPath,
    size_t                                 indexLabel);

  /** Record can generate file level meta data. Typically these go inside
   *  <meta> xml tags. */
  virtual void
  constructXMLMeta(std::ostream&) const;

  /** Record can generate an XML representation of itself for storage.  Typically
   * these go inside <item> xml tags. */
  virtual void
  constructXMLString(std::ostream&, const std::string& indexPath) const;

  /** Set a process id for meta data.  It is not per record and we don't want that
   * because records take up memory. */
  static void
  setProcessName(const std::string& pname)
  {
    theProcessName = pname;
  }

  /** Get the process id for meta data. */
  static const std::string&
  getProcessName()
  {
    return (theProcessName);
  }

  /** Print the contents of record */
  friend std::ostream&
  operator << (std::ostream&, const Record& rec);

private:

  /** Simple index counter used by stock algorithm */
  size_t myIndexCount;

  /** param string list */
  // std::vector<std::vector<std::string> >myParams;
  std::vector<std::string> myParams;

  /** selection string list */
  std::vector<std::string> mySelections;

  /** Record time */
  Time myTime;

  /** Meta data for all records */
  static std::string theProcessName;

public:

  /** Global filter used to trim records before queuing */
  static std::shared_ptr<RecordFilter> theRecordFilter;

  /** Global priority queue used to process Records  */
  static std::shared_ptr<RecordQueue> theRecordQueue;
};

class RecordFilter : public Data
{
public:
  virtual bool
  wanted(const Record& rec){ return true; }
};
}
