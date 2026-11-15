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
 * @brief Decodes NEXRAD Level III Storm Cell (SCIT) Graphic Alphanumeric
 *        Block data into a RAPIO DataTable.
 */
class NIDSStormCellTable : public NIDSSpecializer {
public:

  /** Destructor */
  virtual
  ~NIDSStormCellTable() = default;

  /** Read a Storm Cell DataTable from NIDS stream */
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
   * @brief Stateless helper to extract Storm Cell Packet 8 rows from the
   *        Graphic Alphanumeric Block (skipping the header packet 0 per page).
   */
  static std::vector<std::string>
  extractStormCellLines(
    BlockProductSymbology & s,
    StreamBuffer          & z);

  /**
   * @brief Stateless helper to parse Storm Cell text rows and populate a
   *        DataTable with schema, rows, and geospatial coordinates.
   */
  static std::shared_ptr<DataTable>
  decodeStormCellLines(
    const std::vector<std::string> & lines,
    const LLH                      & radarLoc,
    const Time                     & eventTime,
    const std::string              & typeName);
};
} // namespace rapio
