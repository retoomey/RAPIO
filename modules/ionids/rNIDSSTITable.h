#pragma once

#include "rIONIDS.h"
#include "rNIDSSpecializer.h"

#include <rBlockMessageHeader.h>
#include <rBlockProductDesc.h>
#include <rBlockProductSymbology.h>
#include <rDataTable.h>

#include <memory>
#include <string>
#include <vector>

namespace rapio {
/**
 * @brief Decodes NEXRAD Level III Storm Tracking Information (Product 58)
 *        Graphic Alphanumeric Block data into a RAPIO DataTable.
 */
class NIDSSTITable : public NIDSSpecializer {
public:

  /** Destructor */
  virtual
  ~NIDSSTITable() = default;

  /** Read an STI DataTable from NIDS stream */
  virtual std::shared_ptr<DataType>
  readNIDS(
    IOConfig              & config,
    BlockMessageHeader    & h,
    BlockProductDesc      & d,
    BlockProductSymbology & s,
    StreamBuffer          & z) override;

  /** Write DataType (not implemented for table products) */
  virtual bool
  writeNIDS(
    IOConfig                  & config,
    std::shared_ptr<DataType> dt,
    StreamBuffer              & z) override;

  /** Register this specializer with IONIDS */
  static void
  introduceSelf(IONIDS * owner);

  /**
   * @brief Stateless helper to parse STI text lines and populate a
   *        DataTable with schema, rows, and geospatial coordinates.
   */
  static std::shared_ptr<DataTable>
  decodeSTILines(
    const std::vector<std::string> & lines,
    const LLH                      & radarLoc,
    const Time                     & eventTime,
    const std::string              & typeName);
};
} // namespace rapio
