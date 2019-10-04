#pragma once

#include <rDataType.h>
#include <rDataStore2D.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>

#include <memory>

namespace rapio {
/** DataType for the data of a Netcdf file before any possible
 * delegation.  Note, on delegation the delegated data type
 * might not be a direct subclass.
 *
 * @author Robert Toomey */
class GribDataType : public DataType {
public:
  GribDataType(const std::vector<char>& buf) : myBuf(buf)
  {
    myDataType = "GribData";
  }

  // FIXME: not liking this forced at this level...
  //
  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const override;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const override;

  /** Print the catalog for thie GribDataType.
   * FIXME: better to be able to get a catalog I think */
  void
  printCatalog();

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<DataStore2D<float> >
  get2DData(const std::string& key, const std::string& levelstr);

private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  std::vector<char> myBuf;
};
}
