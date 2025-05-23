#pragma once

#include "rIODataType.h"
#include "rRadialSet.h"
#include "rIOHmrg.h"
#include "rBinaryIO.h"

namespace rapio {
/**
 * Read/Write HMRG Polar binary file
 *
 */
class HmrgRadialSet : public IOSpecializer {
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

  /** Do the heavy work of creating a RadialSet */
  static std::shared_ptr<DataType>
  readRadialSet(StreamBuffer& g, const std::string& radarName);

  /** Do the heavy work of writing a RadialSet as MRMS 2D */
  static bool
  writeRadialSet(StreamBuffer& g, std::shared_ptr<RadialSet> radialset);

  /** Initial introduction of HmrgRadialSet specializer to IOHMRG */
  static void
  introduceSelf(IOHmrg * owner);
};
}
