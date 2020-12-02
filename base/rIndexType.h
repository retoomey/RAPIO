#pragma once
#pragma once

#include <rIndexType.h>
#include <rFAMWatcher.h>
#include <rRecord.h>

namespace rapio {
/**
 * The Index class is a database of metadata records for
 * various input files. The job of an index is to
 * create Records and push them into the job queue.
 *
 * @author Robert Toomey
 */
class IndexType : public IOListener {
public:

  typedef std::vector<std::string> selections_t;

  /** Construct an index */
  IndexType();

  /** Destroy an index */
  virtual
  ~IndexType();

  /** Help function */
  virtual std::string
  getHelpString(){ return ""; }

  /** Every subclass should implement this method as a way to
   *  create a brand-new object.  */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & params,
    const TimeDuration & maximumHistory) = 0;

  /** Handle initial read of data and posting ofrecords. */
  virtual bool
  initialRead(bool realtime, bool archive) = 0;

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
  IndexType(
    const TimeDuration & maximumHistory);

  /** @return the maximum history maintained by this index.  */
  const TimeDuration&
  getMaximumHistory() const
  {
    return (myAgeOffInterval);
  }

protected:

  /** How long to keep back history, if any */
  TimeDuration myAgeOffInterval;

  /** Our index label used to mark recrods */
  size_t myIndexLabel;
};
}
