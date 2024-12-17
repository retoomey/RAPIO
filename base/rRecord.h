#pragma once

#include <rDataType.h>
#include <rTime.h>

#include <memory>
#include <vector>
#include <iostream>

#include <rPTreeData.h>

namespace rapio {
class IndexType;
class RecordFilter;
class RecordQueue;

/** Records are metadata of data we want to read and process.
 * Records store individual information for items in a index.
 * Typically a single DataType object can be created from a single Record.
 * Note: Record fields might not match what you get out of the final
 * read/created DataType. */
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
   * @return invalid-pointer on error.
   */
  std::shared_ptr<DataType>
  createObject() const;

  /** The time of the product referenced by this record. */
  const Time&
  getTime() const
  {
    return (myTime);
  }

  /**
   * DataType (from 2nd selection)
   * Note: This might not match what the builder ends up creating.
   */
  const std::string&
  getDataType() const;

  /** Get the parameter string as a file name.  This is done by 'most'
   * builders, but possibly later we might expand on this. */
  std::string
  getFileName() const;

  /** What is the data source for this record?  For example, netcdf.
   * This is typically parameter 0 */
  std::string
  getDataSourceType() const;

  /** We consider record valid if it at least specifies a builder */
  bool
  isValid() const
  {
    return (myParams.size() > 0);
  }

  /** For a valid record, returns parameters.
   * This is the full unfiltered set of parameters.
   */
  const std::vector<std::string>&
  getParams() const
  {
    return (myParams);
  }

  /** For a valid record, returns selection criteria. */
  const std::vector<std::string>&
  getSelections() const
  {
    return (mySelections);
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

  /** Get the optional source */
  std::string
  getSourceName() const { return mySourceName; }

  /** Set the optional source */
  void
  setSourceName(const std::string& s){ mySourceName = s; }

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

  // XML (FIXME: Should have a rConfigRecord probably)

  /** Read param tag of record. */
  virtual void
  readParams(const std::string & params,
    const std::string          & changes,
    std::vector<std::string>   & v,
    const std::string          & indexPath);

  /** Record can read contents of (typically) an \<item\> tag and fill itself */
  virtual bool
  readXML(const PTreeNode& item,
    const std::string    &indexPath,
    size_t               indexLabel);

  /** Record can generate file level meta data. Typically these go inside
   *  \<meta\> xml tags. */
  virtual void
  constructXMLMeta(std::ostream&) const;

  /** Record can generate an XML representation of itself for storage.  Typically
   * these go inside \<item\> xml tags. */
  virtual void
  constructXMLString(std::ostream&, const std::string& indexPath) const;

  // Matching/sorting

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
  /** Print the contents of record */
  friend std::ostream&
  operator << (std::ostream&, const Record& rec);

private:

  /** Simple index counter used by stock algorithm */
  size_t myIndexCount;

  /** param string list */
  std::vector<std::string> myParams;

  /** selection string list */
  std::vector<std::string> mySelections;

  /** Optional source string, such as KTLX */
  std::string mySourceName;

  /** Record time */
  Time myTime;

  /** Meta data for all records */
  static std::string theProcessName;

public:
  /** The standard format string used by Record for time stamps. */
  static std::string RECORD_TIMESTAMP;

  /** Global filter used to trim records before queuing */
  static std::shared_ptr<RecordFilter> theRecordFilter;

  /** Global priority queue used to process Records  */
  static std::shared_ptr<RecordQueue> theRecordQueue;
};
}
