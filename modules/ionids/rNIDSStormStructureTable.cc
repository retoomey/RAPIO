#include "rNIDSStormStructureTable.h"

#include "rConfigNIDSInfo.h"

#include <rConstants.h>
#include <rError.h>
#include <rProject.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

using namespace rapio;

namespace {
bool
hasRemainingBytes(StreamBuffer & z, size_t needed)
{
  const auto * memBuf = dynamic_cast<const MemoryStreamBuffer *>(&z);

  if (memBuf != nullptr) {
    try {
      memBuf->ensureAvailable(needed);
      return true;
    } catch (const std::out_of_range &) {
      return false;
    }
  }
  return true;
}
} // namespace

void
NIDSStormStructureTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSStormStructureTable>();

  owner->introduce("OrpgStormStructure", io);
  owner->introduce("StormStructure", io);
}

std::shared_ptr<DataType>
NIDSStormStructureTable::readNIDS(
  IOConfig              & /*config*/,
  BlockMessageHeader    & /*h*/,
  BlockProductDesc      & d,
  BlockProductSymbology & s,
  StreamBuffer          & z)
{
  const int productCode = d.getProductCode();
  const auto info       = ConfigNIDSInfo::getNIDSInfo(productCode);

  if (info.isNullProduct()) {
    fLogSevere("Null product not handled, product number: {}", productCode);
    return nullptr;
  }

  const std::string typeName =
    (info.isValid() && !info.getType().empty()) ? info.getType() : "StormStructure";

  std::vector<short> volScanMinutes;
  std::vector<CellTrendPacket> packets;

  extractCellTrendData(s, z, volScanMinutes, packets);

  return decodeStormStructurePackets(
    volScanMinutes, packets, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSStormStructureTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

void
NIDSStormStructureTable::extractCellTrendData(
  BlockProductSymbology        & s,
  StreamBuffer                 & z,
  std::vector<short>           & volScanMinutes,
  std::vector<CellTrendPacket> & packets)
{
  volScanMinutes.clear();
  packets.clear();

  if (s.myBlockID != 3) {
    fLogSevere("Unexpected block ID {} while reading Tabular Alphanumeric Block", s.myBlockID);
    return;
  }

  // 10 bytes of the 128-byte Block 3 header were already consumed into `s`
  // (divider, blockID, blockLength, and the first 2 bytes of embedded MHB).
  // Advance past the remaining 118 bytes of the embedded Message Header and
  // Product Description Blocks to reach the page section.
  if (!hasRemainingBytes(z, 118)) {
    return;
  }
  z.forward(118);

  if (!hasRemainingBytes(z, 2)) {
    return;
  }
  const short divider2 = z.readShort();

  if (divider2 != -1) {
    throw std::runtime_error("Storm Structure stand-alone block doesn't start with divider -1");
  }

  if (!hasRemainingBytes(z, 2)) {
    return;
  }
  const short numPages = z.readShort();

  if (numPages < 0) {
    return;
  }
  for (short pageIdx = 0; pageIdx < numPages; ++pageIdx) {
    while (true) {
      if (!hasRemainingBytes(z, 2)) {
        return;
      }
      const short numChars = z.readShort();
      if (numChars == -1) {
        break;
      }
      if (numChars > 0) {
        if (!hasRemainingBytes(z, static_cast<size_t>(numChars))) {
          return;
        }
        z.forward(static_cast<size_t>(numChars));
      }
    }
  }

  if (!hasRemainingBytes(z, 6)) {
    return;
  }

  // Read CellTrendVolScanTimes (Packet 22)
  try {
    const short packetCode = z.readShort();
    if (packetCode != 22) {
      return;
    }
    /* const short lengthOfBlock = */ z.readShort();
    const auto numVols = static_cast<unsigned char>(z.readChar());
    /* const auto latestVolPtr = */ z.readChar();

    volScanMinutes.reserve(numVols);
    for (unsigned char i = 0; i < numVols; ++i) {
      volScanMinutes.push_back(z.readShort());
    }
  } catch (const std::out_of_range &) {
    return;
  }

  // Read trailing CellTrendDataPackets
  while (hasRemainingBytes(z, 10)) {
    try {
      CellTrendPacket pkt;
      /* const short packetCode   = */ z.readShort();
      /* const short lengthBlock  = */ z.readShort();
      const char id1 = z.readChar();
      const char id2 = z.readChar();
      pkt.stormId = std::string({ id1, id2 });
      pkt.iPosKM  = static_cast<float>(z.readShort()) / 8.0f;
      pkt.jPosKM  = static_cast<float>(z.readShort()) / 8.0f;

      for (int attrIdx = 0; attrIdx < 8; ++attrIdx) {
        const short trendCode = z.readShort();
        const auto numVols    = static_cast<unsigned char>(z.readChar());
        /* const auto latest  = */ z.readChar();

        const float scale =
          ((trendCode == 1) || (trendCode == 2) || (trendCode == 8)) ? 100.0f : 1.0f;

        std::vector<float> vals;
        vals.reserve(numVols);
        for (unsigned char v = 0; v < numVols; ++v) {
          vals.push_back(static_cast<float>(z.readShort() / static_cast<int>(scale)));
        }

        switch (trendCode) {
            case 1: pkt.tops = std::move(vals);
              break;
            case 2: pkt.bases = std::move(vals);
              break;
            case 3: pkt.mxRefHgts = std::move(vals);
              break;
            case 4: pkt.probHail = std::move(vals);
              break;
            case 5: pkt.probSvrHail = std::move(vals);
              break;
            case 6: pkt.vil = std::move(vals);
              break;
            case 7: pkt.mxRefs = std::move(vals);
              break;
            case 8: pkt.centroidHgts = std::move(vals);
              break;
            default: break;
        }
      }

      packets.push_back(std::move(pkt));
    } catch (const std::out_of_range &) {
      break;
    }
  }
} // NIDSStormStructureTable::extractCellTrendData

std::shared_ptr<DataTable>
NIDSStormStructureTable::decodeStormStructurePackets(
  const std::vector<short>           & volScanMinutes,
  const std::vector<CellTrendPacket> & packets,
  const LLH                          & radarLoc,
  const Time                         & eventTime,
  const std::string                  & typeName)
{
  auto table = std::make_shared<DataTable>();

  table->setTypeName(typeName);
  table->setLocation(radarLoc);
  table->setTime(eventTime);
  table->setDataAttributeValue(Constants::ExpiryInterval, "60", "Minutes");

  const std::vector<std::pair<std::string, DataColumn::Type> > schema = {
    { "StormId",            DataColumn::Type::String  },
    { "Top",                DataColumn::Type::Float   },
    { "Base",               DataColumn::Type::Float   },
    { "MxRefHgt",           DataColumn::Type::Float   },
    { "ProbHail",           DataColumn::Type::Float   },
    { "ProbSHail",          DataColumn::Type::Float   },
    { "VIL",                DataColumn::Type::Float   },
    { "MxdBZ",              DataColumn::Type::Float   },
    { "CentrHgt",           DataColumn::Type::Float   },
    { "AlgRank",            DataColumn::Type::Integer },
    { Constants::Latitude,  DataColumn::Type::Float   },
    { Constants::Longitude, DataColumn::Type::Float   },
    { "EpochSeconds",       DataColumn::Type::Integer }
  };

  table->setSchema(schema);

  const Time midnight(
    eventTime.getYear(),
    static_cast<unsigned short>(eventTime.getMonth()),
    static_cast<unsigned short>(eventTime.getDay()),
    0, 0, 0);
  const time_t midnightEpoch = midnight.getSecondsSinceEpoch();

  for (const auto & pkt : packets) {
    if (pkt.tops.size() <= 1) {
      continue; // Not trendable
    }

    const size_t numSteps = std::min({
      pkt.tops.size(),
      pkt.bases.size(),
      pkt.mxRefHgts.size(),
      pkt.probHail.size(),
      pkt.probSvrHail.size(),
      pkt.vil.size(),
      pkt.mxRefs.size(),
      pkt.centroidHgts.size()
    });

    const float rangeKM = std::hypot(pkt.iPosKM, pkt.jPosKM);
    float azDegs        = std::atan2(pkt.iPosKM, pkt.jPosKM) * static_cast<float>(RAD_TO_DEG);
    if (azDegs < 0.0f) {
      azDegs += 360.0f;
    }

    LengthKMs heightKMs = 0.0f;
    AngleDegs eventLat  = 0.0f;
    AngleDegs eventLon  = 0.0f;

    Project::BeamPath_AzRangeToLatLon(
      static_cast<AngleDegs>(radarLoc.getLatitudeDeg()),
      static_cast<AngleDegs>(radarLoc.getLongitudeDeg()),
      azDegs,
      rangeKM,
      0.0f,
      heightKMs,
      eventLat,
      eventLon);

    for (size_t i = 0; i < numSteps; ++i) {
      time_t trendEpoch = (i < volScanMinutes.size()) ?
        (midnightEpoch + static_cast<time_t>(volScanMinutes[i]) * 60) :
        eventTime.getSecondsSinceEpoch();
      if (trendEpoch > eventTime.getSecondsSinceEpoch() + 1800) {
        trendEpoch -= 86400;
      }

      std::map<std::string, DataValue> row;
      row["StormId"]            = pkt.stormId;
      row["Top"]                = pkt.tops[i];
      row["Base"]               = pkt.bases[i];
      row["MxRefHgt"]           = pkt.mxRefHgts[i];
      row["ProbHail"]           = pkt.probHail[i];
      row["ProbSHail"]          = pkt.probSvrHail[i];
      row["VIL"]                = pkt.vil[i];
      row["MxdBZ"]              = pkt.mxRefs[i];
      row["CentrHgt"]           = pkt.centroidHgts[i];
      row["AlgRank"]            = static_cast<int>(i);
      row[Constants::Latitude]  = eventLat;
      row[Constants::Longitude] = eventLon;
      row["EpochSeconds"]       = static_cast<int>(trendEpoch);

      table->addRow(row);
    }
  }

  return table;
} // NIDSStormStructureTable::decodeStormStructurePackets
