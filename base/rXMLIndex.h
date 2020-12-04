#pragma once

#include <rIndexType.h>
#include <rIOXML.h>

namespace rapio {
/**
 * The XMLIndex has a static file of index records,
 * used for an ordered archived dataset
 *
 * @author Robert Toomey
 */
class XMLIndex : public IndexType {
public:

  /** Default constant for a static XML index */
  static const std::string XMLINDEX;

  /** Get help for us */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Can we handle this protocol/path from -i?  Update allowed*/
  static bool
  canHandle(const URL& url, std::string& protocol, std::string& indexparams);

  // ---------------------------------------------------------------------
  // Factory

  /** Introduce to index builder */
  static void
  introduceSelf();

  /** Create a XML index */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  /** Create a new empty XMLIndex, probably as main factory */
  XMLIndex(){ }

  /** Create an individual XMLIndex to a particular location */
  XMLIndex(const URL   & xmlFile,
    const TimeDuration & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  /** Destory an XML index */
  virtual
  ~XMLIndex();
protected:

  /** Location of the data file */
  URL myURL;
};
}
