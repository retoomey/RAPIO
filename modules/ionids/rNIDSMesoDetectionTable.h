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
 * @brief Decodes NEXRAD Level III Mesocyclone Detection (Product 141)
 *        Graphic Alphanumeric Block data into a RAPIO DataTable.
 */
class NIDSMesoDetectionTable : public NIDSSpecializer {
public:

  /** Destructor */
  virtual
  ~NIDSMesoDetectionTable() = default;

  /** Read a Meso Detection DataTable from NIDS stream */
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
   * @brief Stateless helper to parse Meso Detection text lines and populate a
   *        DataTable with schema, rows, and geospatial coordinates.
   */
  static std::shared_ptr<DataTable>
  decodeMesoDetectionLines(
    const std::vector<std::string> & lines,
    const LLH                      & radarLoc,
    const Time                     & eventTime,
    const std::string              & typeName);
};
} // namespace rapio
