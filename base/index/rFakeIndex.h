#pragma once

#include <rIndexType.h>
#include <rIOIndex.h>
#include <rURL.h>

namespace rapio {
/**
 * A index that generates fake records, usually with the
 * 'fake' builder type.  This can be used for testing
 * algorithms and artificial data generation for
 * specialized tests.
 * Our job is to make fake records that call the fake
 * builder, and pass archivie/realtime time along as well
 * as the params given to us.  The builder will use that
 * information to generate fake DataTypes.
 *
 * @author Robert Toomey
 */
class FakeIndex : public IndexType {
public:

  /** Default constant for a fake index */
  static const std::string FAKEINDEX;

  FakeIndex(){ }

  virtual
  ~FakeIndex();

  /** Get help for us */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Can we handle this protocol/path from -i?  Update allowed.
   * FIXME: Not sure 100% yet how to handle this with fake index */
  static bool
  canHandle(const URL& url, std::string& protocol, std::string& indexparams);

  // Factory methods ------------------------------------------

  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  static void
  introduceSelf();

  /** Params are passed to the builder to use to
   * determine what test/datatype to build.
   */
  FakeIndex(const std::string& params,
    const TimeDuration       & maximumHistory);

  // Index methods ------------------------------------------
  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  // IOListener methods ---------------------------------------
  virtual bool
  handlePoll() override;

  /** Generate a fake record */
  virtual bool
  generateRecord(const Time& time);

private:

  /** The params passed to us for sending to builder */
  std::string myParams;

  /** The last created time */
  static Time myLastTime;
};
}
