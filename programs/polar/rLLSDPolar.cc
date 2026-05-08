#include "rLLSDPolar.h"
#include <numeric>
#include <algorithm>
#include <cmath>

using namespace rapio;

// Product keys, not output names.
static const std::string PROD_MEDIAN = "Median";
static const std::string PROD_AZ     = "AzShear";
static const std::string PROD_DIV    = "DivShear";
static const std::string PROD_TOT    = "TotalShear";

namespace {
static bool
abs_compare(double a, double b)
{
  return (std::abs(a) < std::abs(b));
}
}

LLSDPolar::LLSDPolar()
  : myAzGradAzKernel(2500.0f), myAzGradRanKernel(750.0f),
  myDivGradAzKernel(750.0f), myDivGradRanKernel(1500.0f),
  myCressmanKernel(2500.0f), myDoUniform(false),
  myDoSpikeRemoval(false), myStartRangeKM(0.0f)
{
  myDisplayClass = "PolarLLSD";
}

void
LLSDPolar::declareOptions(RAPIOOptions& o)
{
  o.setAuthors("Valliappa Lakshman, Robert Toomey");
  o.setDescription("LLSD polar alg generating Median/Az/Div/Total Shear computation.");
  o.optional("azsize", "2500:750", "kernel az:range (meters) LLSD gradient (cross-radial)");
  o.optional("divsize", "750:1500", "kernal az:range (meters) LLSD gradient (along-radial)");
  o.optional("cressmansize", "2500", "Cressman kernel size (meters)");
  o.optional("startkms", "0.0", "Ignore data closer than this (km)");
  o.boolean("uniform", "Use uniform weights instead of Cressman");
  o.boolean("spikeremoval",
    "Remove radial spikes of high azimuthal shear values that are caused by large areas of improperly dealiased velocity.  Only works with azimuthal shear direction.");

  // Can toggle on/off if wanted. Default is all products
  // o.setDefaultValue("O", "Median AzShear DivShear TotalShear");
  declareProduct(PROD_MEDIAN, "Median filtered product");
  declareProduct(PROD_AZ, "Azimuthal Shear product");
  declareProduct(PROD_DIV, "Divergent Shear product");
  declareProduct(PROD_TOT, "Total Shear product");
}

void
LLSDPolar::processOptions(RAPIOOptions& o)
{
  std::vector<std::string> azK, divK;

  Strings::splitWithoutEnds(o.getString("azsize"), ':', &azK);
  Strings::splitWithoutEnds(o.getString("divsize"), ':', &divK);

  if (azK.size() == 2) {
    myAzGradAzKernel  = std::stof(azK[0]);
    myAzGradRanKernel = std::stof(azK[1]);
  }
  if (divK.size() == 2) {
    myDivGradAzKernel  = std::stof(divK[0]);
    myDivGradRanKernel = std::stof(divK[1]);
  }

  myCressmanKernel = o.getFloat("cressmansize");
  myStartRangeKM   = o.getFloat("startkms");
  myDoUniform      = o.getBoolean("uniform");
  myDoSpikeRemoval = o.getBoolean("spikeremoval");
}

void
LLSDPolar::firstTimeData()
{
  if (myFirstData) {
    myFirstData = false;

    // RadialSets usually wrap in Azimuth (X/dim 0) but not in Range (Y/dim 1)
    // so we setBoundary to wrap for X

    // Initialize Median Filter: percent=50, halfX=1, minFill=0.33
    myMedianFilter = ArrayAlgorithm::create("percent:50:1:0.33:1");
    myMedianFilter->setBoundary(Boundary::Wrap, Boundary::None);
  }
}

void
LLSDPolar::processNewData(RAPIOData& d)
{
  auto rsIn = d.datatype<RadialSet>();

  if (!rsIn) { return; }

  firstTimeData();

  // Normalize to nearest 360 multiple
  // Important currently for wraparound/jitter removal
  // in the filters (since we aren't using ghost cells)
  auto rs = RadialSet::Normalize(rsIn);

  auto results = compute(rs);

  // Note each declared product keys are != typenames
  writeOutputProduct(PROD_MEDIAN, results[PROD_MEDIAN]);
  writeOutputProduct(PROD_AZ, results[PROD_AZ]);
  writeOutputProduct(PROD_DIV, results[PROD_DIV]);
  writeOutputProduct(PROD_TOT, results[PROD_TOT]);
} // LLSDPolar::processNewData

std::map<std::string, std::shared_ptr<RadialSet> >
LLSDPolar::compute(std::shared_ptr<RadialSet> inputin)
{
  // -------------------------------------------------
  // Calculate output product names based on inputin
  // Do it all here so easy to follow.  This is
  // the naming classically used by w2circ
  const std::string name = inputin->getTypeName();
  std::string azname, ranname;
  const std::string totname = name+"_Gradient";
  const std::string medname = name+"_MedianFiltered";
  // 'kinda'  Fails for different velocity names right?
  // maybe we could look at the units instead?
  // Velocity was original and special
  if (Strings::makeLower(name) == "velocity"){ 
     azname = "AzShear";
     ranname = "DivShear";
  }else{
     azname = name+"_AzGradient";
     ranname = name+"_RanGradient";
  }
  const std::string gradientColormap = "Gradient";
  // -------------------------------------------------

  std::map<std::string, std::shared_ptr<RadialSet> > output;

  output[PROD_MEDIAN] = inputin->Clone();
  myMedianFilter->process(inputin->getFloat2D(), output[PROD_MEDIAN]->getFloat2D());
  output[PROD_MEDIAN]->setTypeName(medname);
  output[PROD_MEDIAN]->setColorMapName(inputin->getColorMapName()); 

  output[PROD_AZ] = inputin->Clone(); 
  output[PROD_AZ]->getFloat2D()->fill(Constants::MissingData); // Needed
  output[PROD_AZ]->setUnits("1/m");
  output[PROD_AZ]->setColorMapName(gradientColormap);

  output[PROD_DIV] = output[PROD_AZ]->Clone(); // Unique DIV buffer
  output[PROD_TOT] = output[PROD_AZ]->Clone(); // Unique TOT buffer

  output[PROD_AZ]->setTypeName(azname);
  output[PROD_DIV]->setTypeName(ranname);
  output[PROD_TOT]->setTypeName(totname);

  // Setup pointers to unique buffers
  auto input = output[PROD_MEDIAN];
  auto * data = input->getFloat2DPtr(); // Read-only source
  auto * az   = output[PROD_AZ]->getFloat2DPtr();
  auto * div  = output[PROD_DIV]->getFloat2DPtr();
  auto * tot  = output[PROD_TOT]->getFloat2DPtr();

  const int numRadials = input->getNumRadials();
  const int numGates   = input->getNumGates();

  double avgAzSpacingRad = input->getAzimuthSpacingVector()->ref()[0] * DEG_TO_RAD;
  double distFirstGateM  = input->getDistanceToFirstGateM();
  double gateWidthM      = input->getGateWidthKMs() * 1000.0;

  // Start Range calculation
  size_t startGate =
    (size_t) ((myStartRangeKM * 1000.0 + std::min(myAzGradRanKernel, myDivGradRanKernel)) / gateWidthM);
  int iRanAz = (int) ((0.5 * myAzGradRanKernel) / gateWidthM);
  if (iRanAz < 1) {
    iRanAz = 1;
  }

  int iRanDiv = (int) ((0.5 * myDivGradRanKernel) / gateWidthM);
  if (iRanDiv < 1) {
    iRanDiv = 1;
  }

  // Iterate over the kernel
  SpikeTracker spikeTracker;
  if (myDoSpikeRemoval) {
    spikeTracker.reserve(ABSOLUTE_MAX_GATES);
  }

  for (size_t iRanCurrent = startGate; iRanCurrent < (size_t) numGates; ++iRanCurrent) {

    double ran1         = (iRanCurrent * gateWidthM) + distFirstGateM;
    double radialWidthM = ran1 * avgAzSpacingRad;

    int iAzAz   = std::min(MAX_AZ_HALF_WIDTH, std::max(1, (int) ((0.5 * myAzGradAzKernel) / radialWidthM)));
    int iAzDiv  = std::min(MAX_AZ_HALF_WIDTH, std::max(1, (int) ((0.5 * myDivGradAzKernel) / radialWidthM)));
    int azHalf  = std::max(iAzAz, iAzDiv);
    int ranHalf = std::max(iRanAz, iRanDiv);

    std::shared_ptr<Array<double, 2> > kernelWt = nullptr;
    if (!myDoUniform) {
      kernelWt = precomputeWeights(ranHalf, azHalf, gateWidthM, radialWidthM);
    }

    for (size_t iAzCurrent = 0; iAzCurrent < (size_t) numRadials; ++iAzCurrent) {
      LLSDAccumulator azSolver, divSolver;
      bool breakAz = false, breakDiv = false;

      if (myDoSpikeRemoval) {
        spikeTracker.clear();
      }

      for (int iAz = -azHalf; iAz <= azHalf; ++iAz) {
        int iAz1 = (iAzCurrent + iAz + numRadials) % numRadials;
        for (int iRan = -ranHalf; iRan <= ranHalf; ++iRan) {
          int iRan1 = iRanCurrent + iRan;
          if ((iRan1 < 0) || (iRan1 >= numGates) ) {
            continue;
          }

          double tmpVal = (*data)[iAz1][iRan1];
          bool inAzK    = (std::abs(iAz) <= iAzAz) && (std::abs(iRan) <= iRanAz);
          bool inDivK   = (std::abs(iAz) <= iAzDiv) && (std::abs(iRan) <= iRanDiv);

          if (!Constants::isGood(tmpVal)) {
            if (inAzK) { breakAz = true; }
            if (inDivK) { breakDiv = true; }
            continue;
          }

          double ran2 = (iRan1 * gateWidthM) + distFirstGateM;
          double tx   = iAz * ran2 * avgAzSpacingRad;
          double ty   = ran2 - ran1;
          double wi   = myDoUniform ? 1.0 : (*kernelWt->ptr())[iRan + ranHalf][iAz + azHalf];

          if (inAzK && !breakAz && (wi > 0)) {
            azSolver.accumulate(tx, ty, tmpVal, wi);

            // Gather adjacent neighbors for spike removal
            if (myDoSpikeRemoval) {
              spikeTracker.accumulate(iRan, iAz1, tmpVal);
            }
          }

          if (inDivK && !breakDiv && (wi > 0)) {
            divSolver.accumulate(tx, ty, tmpVal, wi);
          }
        }
      }

      // Spike Removal logging
      if (myDoSpikeRemoval) {
        spikeTracker.detectAndLogSpike(input, iAzCurrent, iRanCurrent);
      }

      // Azimuth shear product
      if (!breakAz) {
        azSolver.solveAzimuthalShear((*az)[iAzCurrent][iRanCurrent]);
      }

      // Divergent shear product
      if (!breakDiv) {
        divSolver.solveRadialDivergence((*div)[iAzCurrent][iRanCurrent]);
      }

      // Total shear product
      float azV  = (*az)[iAzCurrent][iRanCurrent];
      float divV = (*div)[iAzCurrent][iRanCurrent];

      if (Constants::isGood(azV) || Constants::isGood(divV)) {
        double resAz  = Constants::isGood(azV) ? azV : 0;
        double resDiv = Constants::isGood(divV) ? divV : 0;
        (*tot)[iAzCurrent][iRanCurrent] = std::sqrt(resAz * resAz + resDiv * resDiv);
      }
    }
  }

  // Final Pass: Knock down the bad radials
  if (myDoSpikeRemoval) {
    spikeTracker.applySpikeBlankout(az, div, tot);
  }

  return output;
} // LLSDPolar::compute

void
SpikeTracker::detectAndLogSpike(
  std::shared_ptr<rapio::RadialSet> input,
  int iAzCurrent, int iRanCurrent)
{
  // Safety: Ensure we have at least 2 points to find a difference
  // Didn't see this crash in w2circ or here, but it's possible in clear weather
  if (velList1.size() < 2 || velList2.size() < 2 || velList3.size() < 2) {
    return; 
  }
  // Optional but recommended: keep the size-match check if you suspect
  // the kernel might be non-rectangular or clipped by data availability.
  if ((velList1.size() != velList2.size()) || (velList1.size() != velList3.size())) {
    return;
  }

  auto calcStDev = [](const std::vector<double>& v) {
      if (v.empty()) { return 0.0; }
      double mean   = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
      double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0,
          std::plus<double>(), [mean](double a, double b) {
      return (a - mean) * (b - mean);
    });

      return std::sqrt(sq_sum / v.size());
    };

  double meanStDev = (calcStDev(velList1) + calcStDev(velList2) + calcStDev(velList3)) / 3.0;

  if (meanStDev >= 5.0) {
    auto getMaxLoc = [](const std::vector<double>& v) {
        if (v.size() < 2) { return (int) 0; }
        std::vector<double> res(v.size());

        std::adjacent_difference(v.begin(), v.end(), res.begin());
        return (int) std::distance(res.begin(), std::max_element(res.begin() + 1, res.end(), abs_compare));
      };

    int loc1 = getMaxLoc(velList1);
    int loc2 = getMaxLoc(velList2);
    int loc3 = getMaxLoc(velList3);

    if ((loc1 > 0) && (loc1 == loc2) && (loc1 == loc3) ) {
      double diff = std::abs(input->getAzimuthRef()[azList1[loc1]] - input->getAzimuthRef()[azList1[loc1 - 1]]);
      if (diff > 180.0) {
        diff = 360.0 - diff; // Circular wrap check
      }
      auto az_key   = std::make_pair(azList1[loc1 - 1], azList1[loc1]);
      auto gate_loc = std::make_pair(iAzCurrent, iRanCurrent);

      if (diff > 10.0) {
        myBlankoutTable[az_key].push_back(gate_loc);
      } else if (meanStDev >= 10.0) {
        double m11       = std::accumulate(velList1.begin(), velList1.begin() + loc1, 0.0) / loc1;
        double m12       = std::accumulate(velList1.begin() + loc1, velList1.end(), 0.0) / (velList1.size() - loc1);
        double meanDiff1 = std::abs(m11 - m12);

        double m21       = std::accumulate(velList2.begin(), velList2.begin() + loc2, 0.0) / loc2;
        double m22       = std::accumulate(velList2.begin() + loc2, velList2.end(), 0.0) / (velList2.size() - loc2);
        double meanDiff2 = std::abs(m21 - m22);

        double m31       = std::accumulate(velList3.begin(), velList3.begin() + loc3, 0.0) / loc3;
        double m32       = std::accumulate(velList3.begin() + loc3, velList3.end(), 0.0) / (velList3.size() - loc3);
        double meanDiff3 = std::abs(m31 - m32);

        if ((meanDiff1 >= 35.0) || (meanDiff2 >= 35.0) || (meanDiff3 >= 35.0) ) {
          myBlankoutTable[az_key].push_back(gate_loc);
        }
      }
    }
  }
} // SpikeTracker::detectAndLogSpike

void
SpikeTracker::applySpikeBlankout(rapio::ArrayFloat2DPtr az,
  rapio::ArrayFloat2DPtr                                div,
  rapio::ArrayFloat2DPtr                                tot)
{
  for (auto const& pair : myBlankoutTable) {
    // If the total gates flagged for removal are greater than 100, set those gates to RangeFolded
    if (pair.second.size() >= 100) {
      for (auto const& loc : pair.second) {
        // loc.first is iAzCurrent, loc.second is iRanCurrent
        (*az)[loc.first][loc.second]  = rapio::Constants::RangeFolded;
        (*tot)[loc.first][loc.second] = (*div)[loc.first][loc.second];
      }
    }
  }
  myBlankoutTable.clear();
}

std::shared_ptr<Array<double, 2> >
LLSDPolar::precomputeWeights(int x_half, int y_half, double gateWidth, double azWidth)
{
  int x_size   = 2 * x_half + 1;
  int y_size   = 2 * y_half + 1;
  auto kernel  = Arrays::CreateDouble2D(x_size, y_size);
  auto& ref    = kernel->ref();
  double radSq = myCressmanKernel * myCressmanKernel;

  for (int i = 0; i < x_size; ++i) {
    for (int j = 0; j < y_size; ++j) {
      double dRan   = (i - x_half) * gateWidth;
      double dAz    = (j - y_half) * azWidth;
      double distSq = dRan * dRan + dAz * dAz;
      if (distSq >= radSq) { ref[i][j] = 0.0; } else { ref[i][j] = (radSq - distSq) / (radSq + distSq); }
    }
  }
  return kernel;
}

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  LLSDPolar alg = LLSDPolar();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
