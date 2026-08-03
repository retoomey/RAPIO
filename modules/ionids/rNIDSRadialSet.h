#pragma once

#include "rIONIDS.h"
#include "rNIDSSpecializer.h"
#include "rRadialSet.h"
#include "rURL.h"
#include "rBlockRadialSet.h"

#include "rBlockProductDesc.h"

namespace rapio {
/** Handles the read/write of RadialSet DataType from a netcdf file.  */
class NIDSRadialSet : public NIDSSpecializer {
public:

  /** Destructor */
  virtual
  ~NIDSRadialSet();

  /** Read a RadialSet from NIDS data */
  virtual std::shared_ptr<DataType>
  readNIDS(
    IOConfig              & config,
    BlockMessageHeader    & h,
    BlockProductDesc      & d,
    BlockProductSymbology & s,
    StreamBuffer          & z);

  /** Write DataType */
  virtual bool
  writeNIDS(
    IOConfig                  & config,
    std::shared_ptr<DataType> dt,
    StreamBuffer              & z);

  /** Get elevation angle from product description */
  static AngleDegs
  getElevationAngleDegs(BlockProductDesc& d);

  /** Set elevation angle of product description */
  static void
  setElevationAngleDegs(BlockProductDesc& d, AngleDegs elevDegs);

  /** Initial introduction of NIDSRadialSet specializer to IONIDS */
  static void
  introduceSelf(IONIDS * owner);
};
}
