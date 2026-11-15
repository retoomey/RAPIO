/**
 * @file rNEXRADAccumulator.h
 * @brief Accumulator for aggregating NEXRAD Level II radials into complete sweeps and RadialSets.
 *
 * Replicates the sweep accumulation logic of legacy ldm2netcdf. Tracks elevation cuts,
 * beam geometry (azimuths, gate spacings, ranges, beam center adjustments), moment data
 * arrays, and produces standard RAPIO RadialSet objects for downstream MRMS processing.
 */

#pragma once

#include "rNEXRADDecoder.h"
#include <rRadialSet.h>
#include <rDataTable.h>
#include <rMultiDataType.h>
#include <rConfigRadarInfo.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <optional>

namespace rapio {
/**
 * @class NEXRADAccumulator
 * @brief Accumulates incoming decoded Level II radials into complete elevation sweeps
 * and constructs RAPIO RadialSet objects matching the legacy ldm2netcdf format.
 *
 * Supports both static file batch processing and real-time streaming:
 * - Emits immediate sweep completion callbacks via SweepCallback.
 * - Handles SAILS/MRLE elevation cuts correctly.
 * - Tracks radial azimuths with beam center adjustments matching legacy MRMS.
 * - Resolves radar lat/lon/alt from Message 31 metadata or ConfigRadarInfo lookup.
 * - Supports VCP override (-v) and volume start timestamping (-V).
 * @author Robert Toomey
 */
class NEXRADAccumulator {
public:
  /** Callback signature invoked when a full elevation sweep completes */
  using SweepCallback = std::function<void (const std::vector<std::shared_ptr<DataType> >&)>;

  /** @brief Default constructor. */
  NEXRADAccumulator();

  /** @brief Destructor. */
  ~NEXRADAccumulator();

  /**
   * @brief Sets an optional radar name override (e.g. "KATX", "KTLX").
   * @param name The 4-character radar identifier.
   */
  void setRadarName(const std::string& name){ myRadarName = name; }

  const std::string&
  getRadarName() const { return myRadarName; }

  /**
   * @brief Sets a callback for streaming sweep delivery.
   * Invoked immediately when an elevation sweep completes during streaming ingest.
   * @param cb Callback taking a vector of shared_ptr<RadialSet> (one per moment).
   */
  void setSweepCallback(SweepCallback cb){ mySweepCallback = cb; }

  /**
   * @brief Overrides the VCP pattern number (-v option in ldm2netcdf / rL2Stream).
   * @param vcp User-supplied VCP number (0 means agnostic / auto-detect).
   */
  void setUserVCP(int vcp){ myUserVCP = vcp; }

  /**
   * @brief Sets whether to timestamp all elevation sweeps with the volume start time (-V option).
   * @param flag If true, use volume start time; if false, use first radial time of the sweep.
   */
  void setUseVolumeTime(bool flag){ myUseVolumeTime = flag; }

  /**
   * @brief Sets whether to append _EXTRA suffix to SAILS elevation cuts (-e option).
   * @param flag If true, append _EXTRA to TypeName for SAILS cuts.
   */
  void setSeparateSAILS(bool flag){ mySeparateSAILS = flag; }

  bool
  getSeparateSAILS() const { return mySeparateSAILS; }

  /**
   * @brief Sets whether to output tilts lower than 0.5 deg without _EXTRA suffix (-addLower option).
   * @param flag If true, output tilts < 0.5 deg without _EXTRA suffix.
   */
  void setAddLower(bool flag){ myAddLower = flag; }

  bool
  getAddLower() const { return myAddLower; }

  /**
   * @brief Sets the volume start time (from Volume Header or first radial).
   * @param t The volume start timestamp.
   */
  void
  setVolumeStartTime(const Time& t)
  {
    myVolumeStartTime      = t;
    myHaveVolumeStartTime  = true;
    myCurrentStartOfVolume = true;
    myLastWrittenSweepNo   = -1;
    myPrevElevation.clear();
    myPrevSectorSpare.clear();
    myCurrentPRF.clear();
  }

  /**
   * @brief Controls retention of completed RadialSets in memory.
   * For long-running streaming processes, set to false to prevent unbounded memory growth.
   * @param keep If true, keeps completed RadialSets; if false, frees them after callback.
   */
  void setKeepCompletedRadialSets(bool keep){ myKeepCompletedRadialSets = keep; }

  /**
   * @brief Ingests a VCP definition (Message 5).
   * @param vcp Decoded Message 5 VCP.
   */
  void
  setVCP(const NEXRADMessage5VCP& vcp);

  bool
  haveVCP() const { return myHaveVCP; }

  const NEXRADMessage5VCP&
  getVCP() const { return myVCP; }

  /**
   * @brief Ingests an incoming Message 31 radial packet.
   * Detects cut transitions, accumulates moments, and triggers sweep completion.
   * @param radial Decoded Message 31 radial.
   */
  void
  processRadial(const NEXRADMessage31Radial& radial);

  /**
   * @brief Ingests an incoming Message 2 RDA status packet.
   * Constructs a rapio::DataTable matching legacy RDAStatusTable.
   * @param msg2 Decoded Message 2 RDA status.
   */
  void
  processMessage2(const NEXRADMessage2RDAStatus& msg2);

  /**
   * @brief Ingests an incoming Message 3 System/Comm status packet.
   * Constructs a rapio::DataTable matching legacy MSG3DataTable.
   * @param msg3 Decoded Message 3 data.
   */
  void
  processMessage3(const NEXRADMessage3Data& msg3);

  /**
   * @brief Controls output of HVAzNoiseTable (-HVAN).
   * @param flag If true, generates HVAzNoiseTable for surveillance sweeps.
   */
  void setWriteHVAN(bool flag){ myWriteHVAN = flag; }

  bool
  getWriteHVAN() const { return myWriteHVAN; }

  /**
   * @brief Sets azimuthal alignment resolution in degrees (-A).
   * @param res Target resolution in degrees (e.g. 0.5 or 1.0). If <= 0, alignment is disabled.
   */
  void setAlignmentResolution(float res){ myAlignmentResolution = res; }

  float
  getAlignmentResolution() const { return myAlignmentResolution; }

  /**
   * @brief Flushes any accumulated radials in the current sweep, finalizing the sweep.
   */
  void
  flush();

  /**
   * @brief Returns all completed RadialSets across all sweeps (when retention is enabled).
   * @return Vector of completed RadialSet instances.
   */
  const std::vector<std::shared_ptr<RadialSet> >&
  getRadialSets() const
  {
    return myCompletedRadialSets;
  }

  /**
   * @brief Clears all internal state, buffers, and sweep counters.
   */
  void
  reset();

private:
  /** @brief Finalizes the current elevation sweep and triggers callback / storage. */
  void
  finishCurrentSweep();

  /** @brief Materializes any pending Message 2 / Message 3 DataTables using resolved sweep metadata. */
  void
  materializePendingTables();

  /**
   * @brief Constructs a RAPIO RadialSet for a specific moment from accumulated radials.
   * @param rawMomentName Raw Level II moment key (e.g. "REF", "VEL", "SW", "ZDR", "PHI", "RHO").
   * @param rapioTypeName RAPIO moment name (e.g. "Reflectivity", "Velocity").
   * @param units Engineering units string.
   * @return Shared pointer to newly constructed RadialSet.
   */
  std::shared_ptr<RadialSet>
  buildRadialSetForMoment(
    const std::string& rawMomentName,
    const std::string& rapioTypeName,
    const std::string& units);

  /**
   * @brief Constructs HVAzNoiseTable DataTable from accumulated radials for surveillance sweeps.
   * @return Shared pointer to DataTable, or nullptr if not enabled/applicable.
   */
  std::shared_ptr<DataTable>
  buildHVAzNoiseTable();

  /**
   * @brief Resolves radar location (lat, lon, height) using Message 31 metadata or ConfigRadarInfo.
   * @param firstRadial First radial of the sweep.
   * @return Resolved LLH coordinates.
   */
  LLH
  resolveRadarLocation(const NEXRADMessage31Radial& firstRadial);

  std::string myRadarName;
  SweepCallback mySweepCallback;

  // VCP info
  bool myHaveVCP = false;
  NEXRADMessage5VCP myVCP;

  // Sweep tracking
  int myCurrentSweepNo     = -1;
  int myLastWrittenSweepNo = -1;
  float myCurrentElevation = -999.0f;
  std::vector<NEXRADMessage31Radial> myCurrentRadials;
  bool myCurrentStartOfVolume = false;
  bool myCurrentEndOfVolume   = false;

  // Stored completed RadialSets
  std::vector<std::shared_ptr<RadialSet> > myCompletedRadialSets;
  bool myKeepCompletedRadialSets = true;

  // Stored pending DataTables (Message 2 / Message 3)
  std::vector<std::shared_ptr<DataTable> > myPendingDataTables;
  std::optional<NEXRADMessage2RDAStatus> myPendingMsg2;
  std::optional<NEXRADMessage3Data> myPendingMsg3;

  // Multi-PRF and previous elevation tracking per raw moment
  std::map<std::string, float> myPrevElevation;
  std::map<std::string, uint16_t> myPrevSectorSpare;
  std::map<std::string, int> myCurrentPRF;

  // Options matching legacy ldm2netcdf
  int myUserVCP        = 0;
  bool myUseVolumeTime = false;
  Time myVolumeStartTime;
  bool myHaveVolumeStartTime = false;
  bool mySeparateSAILS       = false;
  bool myAddLower  = true;
  bool myWriteHVAN = false;
  float myAlignmentResolution = -1.0f;
};

// Backward compatibility alias
using AR2VAccumulator = NEXRADAccumulator;
} // namespace rapio
