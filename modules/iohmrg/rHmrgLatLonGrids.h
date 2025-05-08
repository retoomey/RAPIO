#pragma once

#include "rIODataType.h"
#include "rIOHmrg.h"
#include "rLatLonGrid.h"
#include "rBinaryIO.h"

namespace rapio {
/**
 * Read/Write HMRG Gridded binary file
 *
 */
class HmrgLatLonGrids : public IOSpecializer {
public:

  /** Read DataType with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt)
  override;

  /** Write DataType with given keys */
  virtual bool
  write(
    std::shared_ptr<DataType>         dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Do the heavy work of reading a LatLonGrid or LatLonHeightGrid */
  static std::shared_ptr<DataType>
  readLatLonGrids(StreamBuffer& g, const int year);

  /** Do the heavy work of writing a LatLonGrid or LatLonHeightGrid */
  static bool
  writeLatLonGrids(StreamBuffer& g, std::shared_ptr<LatLonArea> latlongrid);

  /** Initial introduction of HmrgLatLonGrids specializer to IOHMRG */
  static void
  introduceSelf(IOHmrg * owner);
};
}
