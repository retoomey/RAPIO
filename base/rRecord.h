#pragma once

#include <rDataType.h>
#include <rTime.h>

#include <memory>
#include <vector>
#include <iostream>

#include <rPTreeData.h>

#include <string>

namespace rapio {
class IndexType;
class RecordFilter;
class RecordQueue;

/** Since the entire system works around records, we'll let messages
 * be a superclass.
 *
 * Messages are designed for a small number of short messages.
 * Think of text messages.  If you're trying to send large amounts of data, you
 * probably want a custom DataType instead.
 * Since it's small we use vectors instead o map, which is faster for small N
 *
 * @author Robert Toomey
 */
class Message : public Data {
public:

  /** Create empty message. */
  Message() : myTime(Time::CurrentTime()){ }

  /** Set a value as a string */
  void
  setValue(const std::string& key, const std::string& value)
  {
    // Find the key in myKeys
    for (size_t i = 0; i < myKeys.size(); ++i) {
      if (myKeys[i] == key) {
        // If the key is found, replace its value
        myValues[i] = value; // Convert the value to string
        return;              // Exit after replacing the value
      }
    }

    // If the key is not found, add a new key-value pair
    myKeys.push_back(key);
    myValues.push_back(value);
  }

  /** Get a value as a string */
  bool
  getValue(const std::string& key, std::string& value)
  {
    // Find the key in myKeys
    for (size_t i = 0; i < myKeys.size(); ++i) {
      if (myKeys[i] == key) {
        // If the key is found, replace its value
        value = myValues[i];
      }
    }
    return false;
  }

  #if 0
  /** Copy map of values into our storage */
  void
  setValues(const std::map<std::string, std::string>& inputMap)
  {
    // Clear the existing vectors to ensure they're empty before copying
    myKeys.clear();
    myValues.clear();

    // Iterate through the map and copy keys and values to the vectors
    for (const auto& pair : inputMap) {
      myKeys.push_back(pair.first);
      myValues.push_back(pair.second);
    }
  }

  #endif // if 0
  /** The time of the product referenced by this record. */
  const Time&
  getTime() const
  {
    return (myTime);
  }

  /** The standard time string of the product referenced by this record. */
  std::string
  getTimeString() const
  {
    return (myTime.getString(RECORD_TIMESTAMP));
  }

  /** Set the time of the record */
  void
  setTime(const Time& aTime)
  {
    myTime = aTime;
  }

  /** The index number that made this record */
  size_t
  getIndexNumber() const { return myIndexNumber; }

  /** Set the index number */
  void
  setIndexNumber(size_t i){ myIndexNumber = i; }

  /** Get the optional source */
  std::string
  getSourceName() const { return mySourceName; }

  /** Set the optional source */
  void
  setSourceName(const std::string& s){ mySourceName = s; }

  /** Get the process id for meta data. */
  static const std::string&
  getProcessName()
  {
    return (theProcessName);
  }

  /** Set a process id for meta data.  It is not per record and we don't want that
   * because records take up memory. */
  static void
  setProcessName(const std::string& pname)
  {
    theProcessName = pname;
  }

  /** Set a string in a message */
  void
  setString(const std::string& key, const std::string& value)
  {
    setValue(key, value);
  }

  /** Get a string in a message, return true if found */
  bool
  getString(const std::string& key, std::string& value)
  {
    return getValue(key, value);
  }

  /** Get a string in a message, return "" if not found */
  std::string
  getString(const std::string& key)
  {
    std::string value;

    getValue(key, value);
    return value;
  }

  /** Get keys for us */
  const std::vector<std::string>&
  getKeys() const { return myKeys; }

  /** Get values for us */
  const std::vector<std::string>&
  getValues() const { return myValues; }

  /** Is this a message record? It will go o processNewMessage */
  bool
  isMessage() const { return myKeys.size() > 0; }

  /** The standard format string used by Record for time stamps. */
  static std::string RECORD_TIMESTAMP;

protected:
  /** Record time */
  Time myTime;

  /** Simple index counter used by stock algorithm */
  size_t myIndexNumber;

  /** Optional source string, such as KTLX */
  std::string mySourceName;

  /** Meta data for all records */
  static std::string theProcessName;

private:
  /** Property key list */
  std::vector<std::string> myKeys;

  /** Property value list */
  std::vector<std::string> myValues;
};

/** Records are metadata of data we want to read and process.
 * Records store individual information for items in a index.
 * Typically a single DataType object can be created from a single Record.
 * Note: Record fields might not match what you get out of the final
 * read/created DataType.
 *
 * I've deliberately made this a non-virtual class for memory, since
 * we typically read lots of records. We have a normal record which
 * refers to a builder and data source, and a messsage record for
 * extra communication between algorithsm.*
 * @author Rorbert Toomey */
class Record : public Message {
public:

  /** Creates an empty record. */
  Record();

  /** Create a record from a message.  Since we aren't virtual, this
   * is a promotion. This makes it easier to call methods requiring a
   * record. Note params/selections will be empty in this case, and
   * isData() will return false. */
  Record(const Message& msg) : Message(msg)
  { }

  // FIXME: Called by generateRecord in datatype, it's being passed
  // directly.  We could create message or non message here I think.
  /** Create a record with given time, parameters and selections.  */
  Record(const std::vector<std::string>& params,
    const std::string                  & builder,
    const rapio::Time                  & productTime,
    const std::string                  & dataType,
    const std::string                  & subType);

  /**
   * Create the object referenced by this record.
   * @return invalid-pointer on error.
   */
  std::shared_ptr<DataType>
  createObject() const;

  // ---------------------------------------------
  // Field access
  //


  /** For a valid record, returns parameters.
   * This is the full unfiltered set of parameters.
   */
  const std::vector<std::string>&
  getParams() const
  {
    return (myParams);
  }

  /** Set the params */
  void
  setParams(std::vector<std::string>& in)
  {
    myParams = in;
  }

  /** Return selections tag vector */
  std::vector<std::string>
  getSelections() const
  {
    // Generate from our stuff.
    std::vector<std::string> s;

    s.push_back(myTime.getString(RECORD_TIMESTAMP));
    s.push_back(myDataType);
    s.push_back(mySubType);
    return s;
  }

  /** Set the selections of the record. */
  void
  setSelections(std::vector<std::string>& in)
  {
    // Only called right now by rConfigRecord which reads in say fml/xml
    // We don't use any extra fields right now, and all the indexing was
    // dirty and messy.
    const auto s = in.size();

    if (s > 1) {
      myDataType = in[1];
      if (s > 2) {
        mySubType = in[2];
      }
      // Extra we ignore now for space.  Can use later if really needed.
    }
    #if 0
    mySelections = in;
    if (in.size() > 0) {
      mySelections[0] = myTime.getString(RECORD_TIMESTAMP);
    }
    #endif
  }

  // ---------------------------------------------

  /**
   * DataType (from 2nd selection)
   * Note: This might not match what the builder ends up creating.
   */
  inline const std::string&
  getDataType() const
  {
    return myDataType;
  };

  /**
   * SubType (from 3rd selection)
   * Get the subtype of record
   */
  inline const std::string&
  getSubType() const
  {
    return mySubType;
  }

  /** Get the parameter string as a file name.  This is done by 'most'
   * builders, but possibly later we might expand on this. */
  std::string
  getFileName() const;

  /** What is the data source for this record?  For example, netcdf.
   * This is typically parameter 0 */
  std::string
  getDataSourceType() const;

  /** Is this a data record? It will go o processNewData */
  bool
  isData() const { return myParams.size() > 0; }

  /** Get the standard output name we use.  Note this method
   * isn't overriden in record because we aren't virtual. So it
   * requires explicit type */
  std::string
  getIDString() const;

  /** Sorting of records happens by time, i.e. in chronological order.  */
  friend bool
  operator < (const Record& a,
    const Record          & b);

private:

  /** param string list */
  std::vector<std::string> myParams;

  /** selection string list */
  //  std::vector<std::string> mySelections;

  /** Builder for this record */
  std::string myBuilder;

  /** DataType */
  std::string myDataType;

  /** SubType */
  std::string mySubType;

public:
  /** Global filter used to trim records before queuing */
  static std::shared_ptr<RecordFilter> theRecordFilter;

  /** Global priority queue used to process Records  */
  static std::shared_ptr<RecordQueue> theRecordQueue;
};

/** Print the contents of record */
std::ostream&
operator << (std::ostream&, const rapio::Record& rec);
}
