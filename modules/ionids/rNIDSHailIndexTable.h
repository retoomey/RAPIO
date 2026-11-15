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
 * @brief Decodes NEXRAD Level III Hail Index (Product 59) Graphic Alphanumeric
 *        Block data into a RAPIO DataTable.
 */
class NIDSHailIndexTable : public NIDSSpecializer {
public:

  /** Destructor */
  virtual
  ~NIDSHailIndexTable() = default;

  /** Read a Hail Index DataTable from NIDS stream */
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
   * @brief Stateless helper to extract text lines (Packet 8) from the
   *        Graphic Alphanumeric Block in the stream.
   */
  static std::vector<std::string>
  extractGraphicAlphanumericLines(
    BlockProductSymbology & s,
    StreamBuffer          & z);

  /**
   * @brief Stateless helper to parse Hail Index text lines and populate a
   *        DataTable with schema, rows, and geospatial coordinates.
   */
  static std::shared_ptr<DataTable>
  decodeHailIndexLines(
    const std::vector<std::string> & lines,
    const LLH                      & radarLoc,
    const Time                     & eventTime,
    const std::string              & typeName);

  /**
   * @brief Stateless string tokenizer matching the legacy ICD delimiter logic.
   */
  static std::string
  getSubstr(
    const std::string & str,
    const std::string & startDelimiters,
    const std::string & endDelimiters,
    int               & startIndex,
    int               endIndex = 0);
};
} // namespace rapio
