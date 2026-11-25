#include "r3DSCIT.h"
#include "rIODataType.h"
#include "rDataTable.h"
#include "rConstants.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

using namespace rapio;

extern "C" {
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  return reinterpret_cast<RAPIOAlgorithm *>(new rapio::SCITAlg());
}
}

namespace {
struct GridPoint {
  size_t i    = 0;
  size_t j    = 0;
  size_t k    = 0;
  double refl = 0.0;
};

struct Cluster2D {
  std::vector<GridPoint> gridPt;
  size_t                 id     = 0;
  double                 th     = 0.0;
  double                 bottom = 0.0;
  double                 top    = 0.0;
  double                 maxdbz = Constants::MissingData;
  double                 height = 0.0;
  double                 lat    = Constants::MissingData;
  double                 lon    = Constants::MissingData;
};

constexpr size_t SCIT_NUM_TH = 7;
constexpr std::array<double, SCIT_NUM_TH> SCIT_RTH = { 30.0, 35.0, 40.0, 45.0, 50.0, 55.0, 60.0 };
constexpr double PI = 3.14159265358979323846;
} // namespace

namespace rapio {
void
SCITAlg::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o);
  o.setDescription("Calculates Storm Cell Identification and Tracking (SCIT) features from 3D Reflectivity cubes.");
  o.setAuthors("Ported to RAPIO");
  o.optional("minarea", "15.0", "Minimum storm cell area threshold in km^2 (default 15.0)");
  declareProduct("Scit2D", "Storm cell feature attributes table");
}

void
SCITAlg::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o);
  myMinAreaKm2 = o.getFloat("minarea");
}

void
SCITAlg::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  const Time forTime = input->getTime();

  if ((myScitTable == nullptr) || checkCoverageChange(input)) {
    fLogInfo("Initializing SCIT feature table structures...");
    myScitTable = std::make_shared<DataTable>();
  }

  myScitTable->setTypeName("Scit2D");
  myScitTable->setTime(forTime);
  myScitTable->setLocation(input->getLocation());
  myScitTable->setSchema({
      { "RowName", DataColumn::Type::Integer },
      { "EpochSeconds", DataColumn::Type::Integer },
      { "Latitude", DataColumn::Type::Float },
      { "Longitude", DataColumn::Type::Float },
      { "MaxdBZ", DataColumn::Type::Float },
      { "ReflectivityThreshold", DataColumn::Type::Float },
      { "Height", DataColumn::Type::Float },
      { "FeatureBottomMSL", DataColumn::Type::Float },
      { "FeatureTopMSL", DataColumn::Type::Float },
      { "RadarName", DataColumn::Type::String }
    });
}

class SCITCallback : public LatLonHeightGridCallback {
public:
  SCITCallback(std::shared_ptr<DataTable> scitTable, float minAreaKm2)
    : myTable(std::move(scitTable)), myMinAreaKm2(minAreaKm2){ }

  void
  handleBeginLoop(LatLonHeightGridIterator * /*it*/, const LatLonHeightGrid& grid) override
  {
    myNumLayers = grid.getNumLayers();
    myNumLats   = grid.getNumLats();
    myNumLons   = grid.getNumLons();
    myVolume.assign(myNumLayers * myNumLats * myNumLons, Constants::MissingData);
  }

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    const size_t z = it->getCurrentLayerIdx();
    const size_t y = it->getCurrentLatIdx();
    const size_t x = it->getCurrentLonIdx();

    if ((z < myNumLayers) && (y < myNumLats) && (x < myNumLons)) {
      myVolume[(z * myNumLats + y) * myNumLons + x] = it->getValue();
    }
  }

  void
  handleEndLoop(LatLonHeightGridIterator * /*it*/, const LatLonHeightGrid& grid) override
  {
    if (!myTable || (myNumLayers == 0) || (myNumLats == 0) || (myNumLons == 0)) {
      return;
    }

    const double latSpacing = grid.getLatSpacing();
    const double lonSpacing = grid.getLonSpacing();
    const double nLat       = grid.getLocation().getLatitudeDeg();
    const double sLat       = nLat - static_cast<double>(myNumLats) * latSpacing;
    const double tmpConst   = (nLat - sLat) * PI / 180.0;
    const double pixelArea  = (lonSpacing * 111.0) * (std::cos(tmpConst) * latSpacing * 111.0);

    std::vector<double> heightDiff(myNumLayers, 0.0);

    heightDiff[0] = static_cast<double>(grid.getLayerValue(0));
    for (size_t k = 1; k < myNumLayers; ++k) {
      heightDiff[k] = static_cast<double>(grid.getLayerValue(k) - grid.getLayerValue(k - 1));
    }

    std::vector<Cluster2D> clusters;
    std::vector<size_t> labelGrid(myNumLats * myNumLons, 0);
    size_t numCluster     = 0;
    size_t numClusterPrev = 0;

    for (size_t k = 0; k < myNumLayers; ++k) {
      const double ht = static_cast<double>(grid.getLayerValue(k));
      const size_t sliceOffset = k * myNumLats * myNumLons;

      for (size_t th = 0; th < SCIT_NUM_TH; ++th) {
        std::fill(labelGrid.begin(), labelGrid.end(), 0);

        size_t numClustA = 0;
        size_t numClustB = 0;
        if (th == 0) {
          numClustA      = numCluster;
          numClustB      = numCluster + 1;
          numClusterPrev = numCluster;
        } else {
          numClustA      = numClusterPrev;
          numClustB      = clusters.size();
          numClusterPrev = numClustB;
        }

        for (size_t nc = numClustA; nc < numClustB; ++nc) {
          size_t ncTh = 0;

          const size_t iIdx = (th == 0) ? myNumLons : clusters[nc].gridPt.size();
          const size_t jIdx = (th == 0) ? myNumLats : 1;

          for (size_t jc = 0; jc < jIdx; ++jc) {
            for (size_t ic = 0; ic < iIdx; ++ic) {
              size_t i    = 0;
              size_t j    = 0;
              double refl = Constants::MissingData;

              if (th == 0) {
                i    = ic;
                j    = jc;
                refl = static_cast<double>(myVolume[sliceOffset + j * myNumLons + i]);
              } else {
                i    = clusters[nc].gridPt[ic].i;
                j    = clusters[nc].gridPt[ic].j;
                refl = clusters[nc].gridPt[ic].refl;
              }

              if (!Constants::isGood(static_cast<float>(refl))) {
                continue;
              }

              const size_t cellIdx = j * myNumLons + i;
              if ((labelGrid[cellIdx] == 0) && (refl >= SCIT_RTH[th])) {
                bool addedToExisting = false;

                for (size_t dir = 0; dir < 4; ++dir) {
                  size_t ii = i;
                  size_t jj = j;
                  switch (dir) {
                      case 0:
                        if (j > 0) { jj = j - 1; }
                        break;
                      case 1:
                        if (j + 1 < myNumLats) { jj = j + 1; }
                        break;
                      case 2:
                        if (i > 0) { ii = i - 1; }
                        break;
                      case 3:
                        if (i + 1 < myNumLons) { ii = i + 1; }
                        break;
                  }

                  const size_t nbrIdx = jj * myNumLons + ii;
                  if (!((ii == i) && (jj == j)) && (labelGrid[nbrIdx] >= 1)) {
                    if (labelGrid[cellIdx] == 0) {
                      labelGrid[cellIdx] = labelGrid[nbrIdx];
                      clusters[labelGrid[cellIdx] - 1].gridPt.push_back(GridPoint{ i, j, k, refl });
                    } else if (labelGrid[nbrIdx] != labelGrid[cellIdx]) {
                      const size_t oldID = labelGrid[nbrIdx];
                      const size_t newID = labelGrid[cellIdx];
                      auto& oldPts       = clusters[oldID - 1].gridPt;
                      auto& newPts       = clusters[newID - 1].gridPt;
                      for (const auto& pt : oldPts) {
                        labelGrid[pt.j * myNumLons + pt.i] = newID;
                        newPts.push_back(pt);
                      }
                      oldPts.clear();
                    }
                    addedToExisting = true;
                  }
                }

                if (!addedToExisting) {
                  numCluster++;
                  ncTh++;
                  labelGrid[cellIdx] = numCluster;

                  Cluster2D clust;
                  clust.id     = numCluster;
                  clust.th     = SCIT_RTH[th];
                  clust.height = ht;
                  clust.bottom = ht - (heightDiff[k] / 2.0);
                  clust.top    = ht + (heightDiff[k] / 2.0);
                  clust.gridPt.push_back(GridPoint{ i, j, k, refl });
                  clusters.push_back(std::move(clust));
                }
              }
            }
          }

          // Filter out clusters smaller than the minimum area threshold
          const size_t startIdx = (clusters.size() >= ncTh) ? (clusters.size() - ncTh) : 0;
          for (size_t l = startIdx; l < clusters.size(); ++l) {
            const size_t numpts = clusters[l].gridPt.size();
            if ((numpts > 0) && ((static_cast<double>(numpts) * pixelArea) < static_cast<double>(myMinAreaKm2))) {
              for (const auto& pt : clusters[l].gridPt) {
                labelGrid[pt.j * myNumLons + pt.i] = 0;
              }
              clusters[l].gridPt.clear();
              if ((th > 0) && (ncTh > 0)) {
                ncTh--;
              }
            }
          }

          // Compress clusters: if higher-dBZ sub-clusters exist, remove parent lower-dBZ cluster
          if ((nc < clusters.size()) && (ncTh > 0) && (th > 0) && !clusters[nc].gridPt.empty()) {
            clusters[nc].gridPt.clear();
          }
        }
      }
    }

    const int epochSec = static_cast<int>(grid.getTime().getSecondsSinceEpoch());

    for (auto& clust : clusters) {
      if (clust.gridPt.empty()) {
        continue;
      }

      double iSum   = 0.0;
      double jSum   = 0.0;
      double rSum   = 0.0;
      double maxDbz = clust.gridPt[0].refl;

      for (const auto& pt : clust.gridPt) {
        if (pt.refl > maxDbz) {
          maxDbz = pt.refl;
        }
        iSum += pt.refl * static_cast<double>(pt.i);
        jSum += pt.refl * static_cast<double>(pt.j);
        rSum += pt.refl;
      }

      clust.maxdbz = maxDbz;

      size_t i1 = 0;
      size_t j1 = 0;
      if (rSum > 0.0) {
        i1 = static_cast<size_t>(std::llround(iSum / rSum));
        j1 = static_cast<size_t>(std::llround(jSum / rSum));
        i1 = std::min(i1, myNumLons - 1);
        j1 = std::min(j1, myNumLats - 1);
      }

      const LLH centroidLoc = grid.getCenterLocationAt(j1, i1);
      clust.lat = centroidLoc.getLatitudeDeg();
      clust.lon = centroidLoc.getLongitudeDeg();

      myTable->addRow({
          { "RowName", static_cast<int>(clust.id) },
          { "EpochSeconds", epochSec },
          { "Latitude", static_cast<float>(clust.lat) },
          { "Longitude", static_cast<float>(clust.lon) },
          { "MaxdBZ", static_cast<float>(clust.maxdbz) },
          { "ReflectivityThreshold", static_cast<float>(clust.th) },
          { "Height", static_cast<float>(clust.height) },
          { "FeatureBottomMSL", static_cast<float>(clust.bottom) },
          { "FeatureTopMSL", static_cast<float>(clust.top) },
          { "RadarName", std::string("Multi") }
        });
    }
  } // handleEndLoop

private:
  std::shared_ptr<DataTable> myTable;
  float myMinAreaKm2 = 15.0f;
  size_t myNumLayers = 0;
  size_t myNumLats   = 0;
  size_t myNumLons   = 0;
  std::vector<float> myVolume;
};

std::unique_ptr<LatLonHeightGridCallback>
SCITAlg::createCallback()
{
  return std::make_unique<SCITCallback>(myScitTable, myMinAreaKm2);
}

void
SCITAlg::writeFinalProducts(RAPIOAlgorithm * writer)
{
  if (writer && myScitTable) {
    writer->writeOutputProduct("Scit2D", myScitTable);
  }
}
} // namespace rapio
