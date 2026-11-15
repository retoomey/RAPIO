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
 * @brief Decodes NEXRAD Level III Storm Structure (Product 62)
 *        Stand-Alone Tabular Alphanumeric Block + Cell Trend Data Packets
 *        into a RAPIO DataTable.
 */
class NIDSStormStructureTable : public NIDSSpecializer {
public:

  struct CellTrendPacket {
    std::string        stormId;
    float              iPosKM = 0.0f;
    float              jPosKM = 0.0f;
    std::vector<float> tops;
    std::vector<float> bases;
    std::vector<float> mxRefHgts;
    std::vector<float> probHail;
    std::vector<float> probSvrHail;
    std::vector<float> vil;
    std::vector<float> mxRefs;
    std::vector<float> centroidHgts;
  };

  /** Destructor */
  virtual
  ~NIDSStormStructureTable() = default;

  /** Read a Storm Structure DataTable from NIDS stream */
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
   * @brief Stateless helper to extract cell trend scan times (minutes after midnight)
   *        and cell trend data packets following the stand-alone Tabular Alphanumeric Block.
   */
  static void
  extractCellTrendData(
    BlockProductSymbology        & s,
    StreamBuffer                 & z,
    std::vector<short>           & volScanMinutes,
    std::vector<CellTrendPacket> & packets);

  /**
   * @brief Stateless helper to populate a DataTable from parsed CellTrendPackets.
   */
  static std::shared_ptr<DataTable>
  decodeStormStructurePackets(
    const std::vector<short>           & volScanMinutes,
    const std::vector<CellTrendPacket> & packets,
    const LLH                          & radarLoc,
    const Time                         & eventTime,
    const std::string                  & typeName);
};
} // namespace rapio
