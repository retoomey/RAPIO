#pragma once

#include <RAPIO.h>

#include <map>
#include <vector>
#include <string>

namespace rapio {
/**
 * @class SpikeTracker
 * @brief Detects and removes anomalous radial spikes in radar data, typically caused by velocity dealiasing errors.
 * * @details Dealiasing failures often present as narrow, high-variance radial streaks in the velocity
 * field. When the Linear Least Squares Derivative (LLSD) algorithm processes these streaks, it can
 * produce artificially extreme azimuthal shear values along the edges of the bad data.
 * * This class mitigates those artifacts using a two-pass approach:
 * 1. **Detection (Gate-by-Gate):** As the algorithm loops over the radar volume, this class accumulates
 * velocities from the immediate range neighbors (-1, 0, +1). If the local standard deviation and
 * azimuthal differences exceed configured thresholds, the gate is flagged in an internal lookup table.
 * 2. **Blankout (Post-Processing):** After the entire elevation scan is processed, the algorithm makes
 * a final pass to overwrite the flagged outlier gates with a RangeFolded mask, preventing false
 * shear signatures from polluting the final product.
 *
 * @author Valliappa Lakshmanan (Lak)
 * @author Robert Toomey
 */
class SpikeTracker {
public:
  /** Reserve memory to help the loop some */
  inline void
  reserve(int maxGates)
  {
    velList1.reserve(maxGates);
    velList2.reserve(maxGates);
    velList3.reserve(maxGates);
    azList1.reserve(maxGates);
  }

  /** Clear for another pass */
  inline void
  clear()
  {
    velList1.clear();
    velList2.clear();
    velList3.clear();
    azList1.clear();
  }

  /** Accumulate values for blanking out */
  inline void
  accumulate(int iRan, int iAz1, double tmpVal)
  {
    if (std::abs(iRan) <= 1) {
      if (iRan == 1) {
        velList1.push_back(tmpVal);
        azList1.push_back(iAz1);
      } else if (iRan == 0) {
        velList2.push_back(tmpVal);
      } else if (iRan == -1) {
        velList3.push_back(tmpVal);
      }
    }
  }

  /** Perform statistical math and detect radial spikes */
  void
  detectAndLogSpike(std::shared_ptr<rapio::RadialSet> input,
    int                                               iAzCurrent,
    int                                               iRanCurrent);

  /** Apply RangeFolded masks to the detected spike table */
  void
  applySpikeBlankout(rapio::ArrayFloat2DPtr az,
    rapio::ArrayFloat2DPtr                  div,
    rapio::ArrayFloat2DPtr                  tot);

private:
  using GatePair    = std::pair<int, int>;
  using BlankoutMap = std::map<GatePair, std::vector<GatePair> >;

  BlankoutMap myBlankoutTable;
  std::vector<double> velList1, velList2, velList3;
  std::vector<int> azList1;
};

/**
 * @class LLSDAccumulator
 * @brief Accumulates spatial and velocity data to solve a 2D Weighted Linear Least Squares Derivative (LLSD).
 * * @details This class is responsible for gathering the necessary summations to perform a
 * 2D plane fit over a localized kernel of radar data. The plane equation is modeled as:
 * V(x,y) = c1*x + c2*y + c3
 * * By accumulating the weighted variances and covariances of the spatial distances (x, y)
 * and the radar velocities (u), this class builds the 3x3 covariance matrix and 3x1
 * solution vector required to extract the localized shear (c1) and divergence (c2) slopes.
 *
 * @author Valliappa Lakshmanan (Lak)
 * @author Robert Toomey
 */
class LLSDAccumulator {
private:
  /** Sum of the weights (\Sigma w) */
  double w = 0;
  /** Sum of weighted azimuthal distances (\Sigma w*x) */
  double x = 0;
  /** Sum of weighted radial distances (\Sigma w*y) */
  double y = 0;
  /** Sum of weighted azimuthal variances (\Sigma w*x^2) */
  double x2 = 0;
  /** Sum of weighted radial variances (\Sigma w*y^2) */
  double y2 = 0;
  /** Sum of weighted spatial covariance (\Sigma w*x*y) */
  double xy = 0;
  /** Sum of weighted velocities (\Sigma w*u) */
  double u = 0;
  /** Sum of weighted velocity-azimuth cross-products (\Sigma w*x*u) */
  double xu = 0;
  /** Sum of weighted velocity-range cross-products (\Sigma w*y*u) */
  double yu = 0;
public:

  /**
   * @brief Accumulates a single radar gate's data into the least squares matrix.
   * * @param tx  The physical azimuthal distance of the gate from the kernel center (meters).
   * @param ty  The physical radial (range) distance of the gate from the kernel center (meters).
   * @param vel The measured radar velocity at this gate.
   * @param wi  The spatial weight applied to this gate (e.g., Cressman weight or 1.0 for uniform).
   */
  inline void
  accumulate(double tx, double ty, double vel, double wi)
  {
    w  += wi;
    xu += tx * vel * wi;
    yu += ty * vel * wi;
    u  += vel * wi;

    #if 0
    x2 += tx * tx * wi;
    xy += tx * ty * wi;
    x  += tx * wi;
    y  += ty * wi;
    y2 += ty * ty * wi;
    #else
    // Original w2circ doesn't multiply by the weight which appears to be a bug
    // However I think for 'uniform' weights wi was always 1.  For cressmen I think
    // we actually need to add the * wi above.  For now I'll leave it as it was
    x2 += tx * tx;
    xy += tx * ty;
    x  += tx;
    y  += ty;
    y2 += ty * ty;
    #endif // if 0
  }

  /** Solves the matrix for the X-derivative. Note we solve using
   * double precision then clamp to our output float at end */
  inline void
  solveAzimuthalShear(float& out) const
  {
    double D = -(w * xy * xy) + (w * x2 * y2) - (x * x * y2)
      + (2 * xy * x * y) - (x2 * y * y);

    if (std::abs(D) > 1e-9) {
      double a11 = (w * y2 - y * y) / D;
      double a12 = (x * y - w * xy) / D;
      double a13 = (xy * y - x * y2) / D;
      out = static_cast<float>(xu * a11 + yu * a12 + u * a13);
    }
  }

  /** Solves the matrix for the Y-derivative. Note we solve using
   * double precision then clamp to our output float at end */
  inline void
  solveRadialDivergence(float& out) const
  {
    double D = -(w * xy * xy) + (w * x2 * y2) - (x * x * y2)
      + (2 * xy * x * y) - (x2 * y * y);

    if (std::abs(D) > 1e-9) {
      double a21 = (x * y - w * xy) / D;
      double a22 = (x2 * w - x * x) / D;
      double a23 = (x * xy - x2 * y) / D;
      out = static_cast<float>(xu * a21 + yu * a22 + u * a23);
    }
  }
};

/**
 * @class LLSDPolar
 * @brief Based off of MRMS w2img::PolarLLSD and parts of w2circ,
 * an optimized 4 product generator.
 * Computes Median, AzShear, DivShear, and TotalShear from RadialSets.
 *
 * Debating this being in image/filters, but that API still being
 * developed at the time of this.
 *
 * @author Valliappa Lakshmanan (Lak)
 * @author Robert Toomey
 *
 */
class LLSDPolar : public rapio::RAPIOAlgorithm {
public:

  /** Hard cap on the azimutal kernel half-width to prevent runaway processing
   * close to radar center */
  static constexpr int MAX_AZ_HALF_WIDTH = 25;

  // The absolute maximum number of gates in a radial slice of the kernel
  static constexpr int ABSOLUTE_MAX_GATES = (2 * MAX_AZ_HALF_WIDTH) + 1;

  /** Create the LLSDPolar algorithm */
  LLSDPolar();

  /** Destroy LLSDPolar algorithm */
  virtual
  ~LLSDPolar() = default;

  /** Declare program options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process and validate program options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process new data coming in */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:

  /** Initialize filters we'll use */
  void
  firstTimeData();

  /** Core computation ported from w2img_PolarLLSD.cc */
  std::map<std::string, std::shared_ptr<RadialSet> >
  compute(std::shared_ptr<RadialSet> input);

  /** Uniform weighting caching */
  std::shared_ptr<Array<double, 2> >
  precomputeWeights(int x_half, int y_half, double gateWidth, double azWidth);

private:
  float myAzGradAzKernel;
  float myAzGradRanKernel;
  float myDivGradAzKernel;
  float myDivGradRanKernel;
  float myCressmanKernel;
  bool myDoUniform;
  bool myDoSpikeRemoval;
  float myStartRangeKM;

  /** First time we have gotten data */
  bool myFirstData = true;

  /** Stored median filter */
  std::shared_ptr<ArrayAlgorithm> myMedianFilter;
};
} // namespace rapio
