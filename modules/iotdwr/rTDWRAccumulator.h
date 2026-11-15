/**
 * @file rTDWRAccumulator.h
 * @brief Accumulator for aggregating TDWR radials into complete sweeps and RadialSets.
 *
 * Implements the TDWR sweep accumulation logic replacing legacy ldm2netcdf TDWRAccumulator:
 * - Monitors SOE, EOE, SOV, and EOV flags to trigger sweep completion.
 * - Scales raw gate bytes into meteorological moments (Reflectivity, ConditionedVelocity,
 *   AliasedVelocity, SpectrumWidth, SignalToNoise, ReflectivityLowPRF, SignalToNoiseLowPRF).
 * - Expands Low PRF 300m gates (duplicating gates) into uniform 150m spacing.
 * - Applies magnetic declination rotation to radial azimuths.
 * - Computes and attaches Nyquist velocities from radar frequency and PRI.
 * - Resolves radar coordinates, frequency, and magnetic declination from ConfigRadarInfo.
 */

#pragma once

#include "rTDWRDecoder.h"
#include <rRadialSet.h>
#include <rConfigRadarInfo.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace rapio {
/**
 * @class TDWRAccumulator
 * @brief Accumulates incoming decoded TDWR radials into complete elevation sweeps
 * and constructs standard RAPIO RadialSet objects matching legacy ldm2netcdf.
 */
class TDWRAccumulator {
public:
  /** Callback signature invoked when a full elevation sweep completes */
  using SweepCallback = std::function<void (const std::vector<std::shared_ptr<DataType> >&)>;

  /** @brief Default constructor. */
  TDWRAccumulator();

  /** @brief Destructor. */
  ~TDWRAccumulator();

  /**
   * @brief Sets the radar identifier (e.g. "TOKC", "TDFW").
   * @param name Radar name.
   */
  void setRadarName(const std::string& name){ myRadarName = name; }

  const std::string&
  getRadarName() const { return myRadarName; }

  /**
   * @brief Sets a callback for streaming sweep delivery.
   * Invoked immediately when an elevation sweep completes.
   * @param cb Callback taking a vector of shared_ptr<DataType> (RadialSets).
   */
  void setSweepCallback(SweepCallback cb){ mySweepCallback = cb; }

  /**
   * @brief Overrides the VCP pattern number.
   * @param vcp User-supplied VCP number (0 means auto-detect from scan strategy).
   */
  void setUserVCP(int vcp){ myUserVCP = vcp; }

  int
  getUserVCP() const { return myUserVCP; }

  /**
   * @brief Sets whether to timestamp all elevation sweeps with the volume start time (-V option).
   * @param flag If true, use volume start time; if false, use first radial time.
   */
  void setUseVolumeTime(bool flag){ myUseVolumeTime = flag; }

  bool
  getUseVolumeTime() const { return myUseVolumeTime; }

  /**
   * @brief Sets the volume start timestamp.
   * @param t Volume start time.
   */
  void
  setVolumeStartTime(const Time& t)
  {
    myVolumeStartTime     = t;
    myHaveVolumeStartTime = true;
    myStartOfVolumeIsSet  = true;
  }

  Time
  getVolumeStartTime() const { return myVolumeStartTime; }

  /**
   * @brief Sets the noise threshold for Low PRF reflectivity filtering (default 1.0 dB).
   * @param threshold SNR threshold in dB.
   */
  void  setNoiseThreshold(float threshold){ myNoiseThreshold = threshold; }

  float
  getNoiseThreshold() const { return myNoiseThreshold; }

  /**
   * @brief Sets the magnetic declination offset in degrees.
   * @param offsetDegs Magnetic offset in degrees (e.g. -10.0 for 10W, +7.0 for 7E).
   */
  void
  setMagneticOffset(float offsetDegs);

  /**
   * @brief Sets the magnetic declination offset from a string (e.g. "10W", "7E", "-5.5").
   * @param offsetStr Formatted magnetic declination string.
   */
  void
  setMagneticOffset(const std::string& offsetStr);
  float
  getMagneticOffset() const;

  /**
   * @brief Sets the radar transmitter frequency in MHz (default 5603 MHz).
   * @param freqMHz Radar frequency in MHz.
   */
  void
  setFrequency(int freqMHz);
  int
  getFrequency() const;

  /**
   * @brief Controls retention of completed RadialSets in memory.
   * @param keep If true, keep completed RadialSets; if false, discard after callback.
   */
  void setKeepCompletedRadialSets(bool keep){ myKeepCompletedRadialSets = keep; }

  bool
  getKeepCompletedRadialSets() const { return myKeepCompletedRadialSets; }

  /**
   * @brief Sets azimuthal alignment resolution in degrees (-A).
   * @param res Target resolution in degrees (e.g. 0.5 or 1.0). If <= 0, alignment is disabled.
   */
  void setAlignmentResolution(float res){ myAlignmentResolution = res; }

  float
  getAlignmentResolution() const { return myAlignmentResolution; }

  /**
   * @brief Ingests an incoming Message 0x2b00 (Compressed Base Data) radial.
   * @param radial Decoded compressed radial.
   */
  void
  processRadial(const TDWRCompressedRadial& radial);

  /**
   * @brief Ingests an incoming Message 0x2b01 (Low PRF Base Data) radial.
   * @param radial Decoded Low PRF radial.
   */
  void
  processRadial(const TDWRLowPRFRadial& radial);

  /**
   * @brief Flushes any accumulated radials in current sweep, triggering completion.
   */
  void
  flush();

  /**
   * @brief Returns all completed RadialSets across all sweeps.
   * @return Vector of completed RadialSet instances.
   */
  const std::vector<std::shared_ptr<RadialSet> >&
  getRadialSets() const
  {
    return myCompletedRadialSets;
  }

  /**
   * @brief Clears all internal state and accumulated radials.
   */
  void
  reset();

  /**
   * @brief Helper to parse magnetic declination string (e.g. "10W" -> -10.0f, "7E" -> +7.0f).
   * @param str Magnetic declination string.
   * @return Parsed angle in degrees.
   */
  static float
  parseMagneticOffsetString(const std::string& str);

private:
  /** @brief Finalizes the current elevation sweep and triggers callback / storage. */
  void
  finishCurrentSweep();

  /** @brief Constructs RadialSets for Message 0x2b00 (Compressed Base Data). */
  std::vector<std::shared_ptr<RadialSet> >
  buildCompressedRadialSets();

  /** @brief Constructs RadialSets for Message 0x2b01 (Low PRF Base Data). */
  std::vector<std::shared_ptr<RadialSet> >
  buildLowPRFRadialSets();

  /** @brief Resolves radar location (lat, lon, height). */
  LLH
  resolveRadarLocation();

  /** @brief Resolves effective magnetic offset in degrees. */
  float
  resolveMagneticOffset();

  /** @brief Resolves effective radar frequency in MHz. */
  int
  resolveFrequency();

  std::string myRadarName;
  SweepCallback mySweepCallback;

  // Options & configuration
  int myUserVCP        = 0;
  bool myUseVolumeTime = false;
  Time myVolumeStartTime;
  bool myHaveVolumeStartTime  = false;
  bool myStartOfVolumeIsSet   = false;
  float myNoiseThreshold      = 1.0f;
  float myMagneticOffset      = 0.0f;
  bool myHaveExplicitMagnetic = false;
  int myFrequency = 5603; ///< Typical TDWR frequency 5603 MHz
  bool myHaveExplicitFrequency   = false;
  bool myKeepCompletedRadialSets = true;
  float myAlignmentResolution    = -1.0f;

  // Sweep accumulation state
  int myCurrentScanNumber     = -1;
  int myCurrentScanStrategy   = -1;
  float myCurrentElevation    = -999.0f;
  bool myCurrentStartOfVolume = false;
  bool myCurrentEndOfVolume   = false;
  std::vector<TDWRCompressedRadial> myCurrentCompressedRadials;
  std::vector<TDWRLowPRFRadial> myCurrentLowPRFRadials;

  // Stored completed RadialSets
  std::vector<std::shared_ptr<RadialSet> > myCompletedRadialSets;
};
} // namespace rapio
