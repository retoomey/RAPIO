#pragma once
#pragma once

#include <rIndexType.h>
#include <rFAMWatcher.h>
#include <rRecord.h>

namespace rapio {
/** A listener for getting record events from an index */
class IndexListener : public Event {
public:

  virtual void
  notifyNewRecordEvent(const Record& item) = 0;
  virtual void
  notifyEndDatasetEvent(const Record& item) = 0;
};

/**
 * The Index class is a database of metadata records for
 * various input files
 */
class IndexType : public IOListener {
public:

  typedef std::vector<std::string> selections_t;

  /** Construct an index */
  IndexType();

  /** Destroy an index */
  virtual
  ~IndexType();

  /** Every subclass should implement this method as a way to
   *  create a brand-new object.  */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string                            & protocol,
    const URL                                    & location,
    std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                           & maximumHistory) = 0;

  /** Handle initial read of data and posting ofrecords. */
  virtual bool
  initialRead(bool realtime) = 0;

  /** The URL location for where the data is coming from */
  const URL&
  getURL() const
  {
    return (myURL);
  }

  /** Set the URL of the index */
  void
  setURL(const URL& u)
  {
    myURL = u;
  }

  /** Get the index label used to mark new records */
  size_t
  getIndexLabel()
  {
    return myIndexLabel;
  }

  /** Set the index label used to mark new records */
  void
  setIndexLabel(size_t l)
  {
    myIndexLabel = l;
  }

  /** Add an index listener to get special index events */
  void
  addIndexListener(std::shared_ptr<IndexListener> l);

  /** Get number of listeners */
  size_t
  getNumListeners();

  /**
   * Forms the key to which the given selection criteria would be
   * mapped. Uses only the criteria in the range [offset1,offset2)
   * (the second not inclusive).
   *
   * @param selection the selection criteria
   * @param offset [optional] which criteria to use. By default, start
   * at zero (use all).
   */
  virtual std::string
  formKey(const selections_t& sel,
    size_t                  offset1 = 0,
    size_t                  offset2 = ~0) const;

  /**
   * Users will normally create a subclass of Index, such
   * as XMLIndex, etc.
   */
  IndexType(std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                                   & maximumHistory);

  /** @return the maximum history maintained by this index.  */
  const TimeDuration&
  getMaximumHistory() const
  {
    return (myAgeOffInterval);
  }

  /** Process a record in index, reading data and notifying all listeners. */
  void
  processRecord(const Record& item);

protected:

  /** How long to keep back history, if any */
  TimeDuration myAgeOffInterval;

  /** Every index has a URL representing its location */
  URL myURL;

  /** Our listeners. */
  std::vector<std::shared_ptr<IndexListener> > myListeners;

  /** Our index label used to mark recrods */
  size_t myIndexLabel;

public:

  /**
   * Notify interested parties of a new Record to the index
   */
  virtual void
  notifyNewRecordEvent(const Record& item);

  /**
   * Notify interested parties that this data set is complete and
   * that no new Records will come from this data set.
   */
  virtual void
  notifyEndDatasetEvent(const Record& item);
};
}
