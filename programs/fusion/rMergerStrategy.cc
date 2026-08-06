#include "rMergerStrategy.h"
#include "rProcessTimer.h"
#include "rArith.h"

using namespace rapio;

// ============================================================================
// Weighted Average Strategy (Legacy mergeTo)
// ============================================================================
void 
WeightedAverageStrategy::reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> cache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision)
{
  auto& obsManager = db->getObservationManager();
  
  if (obsManager.getPayloadSize() != 2 && obsManager.getPayloadSize() != 0) {
      fLogSevere("WeightedAverageStrategy requires payload size 2, got {}.", obsManager.getPayloadSize());
      return;
  }

  ProcessTimer test("Strategy: Merging XYZ tree (Weighted Average)");
  cache->fillPrimary(Constants::DataUnavailable);
  
  const size_t gridZ = cache->getNumLayers();
  const size_t gridY = cache->getNumLats();
  const size_t gridX = cache->getNumLons();
  const auto& haves = db->getHaves();
  const auto& missings = db->getMissings();

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    auto w = output->getFloat2D("weights");
    w->fill(0);
    auto& wa = output->getFloat2DRef("weights");
    auto gridtestP = output->getFloat2D();
    gridtestP->fill(0);
    auto& gridtest = output->getFloat2DRef();

    for (auto it = obsManager.begin(); it != obsManager.end(); ++it) {
      auto r = std::static_pointer_cast<PayloadSourceList<2>>(it->second);
      for (auto& v : r->myObs[z]) {
        const int atX = v.x - offsetX;
        const int atY = v.y - offsetY;
        if ((atX < 0) || (atY < 0) || (atX >= static_cast<int>(gridX)) || (atY >= static_cast<int>(gridY))) {
          continue;
        }
        gridtest[atY][atX] += v.data[0];
        wa[atY][atX]       += v.data[1];
      }
    }
  }

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    auto& wa = output->getFloat2DRef("weights");
    auto& gridtest = output->getFloat2DRef();

    for (size_t x = 0; x < gridX; x++) {
      for (size_t y = 0; y < gridY; y++) {
        auto& v = gridtest[y][x];
        auto& w = wa[y][x];
        if (w == 0) {
          const size_t globalX = offsetX + x;
          const size_t globalY = offsetY + y;
          if (missings[haves.getIndex3D(globalX, globalY, z)] >= cutoff) {
            v = Constants::MissingData;
          } else {
            v = Constants::DataUnavailable;
          }
          continue;
        }
        v /= w;
        if (precision > 0) v = Arith::roundOff(v, precision);
      }
    }
  }
  fLogInfo("{}", test);
}

// ============================================================================
// Maximum Value Strategy (Legacy maxTo)
// ============================================================================
void 
MaximumValueStrategy::reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> cache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision)
{
  auto& obsManager = db->getObservationManager();
  
  if (obsManager.getPayloadSize() != 2 && obsManager.getPayloadSize() != 0) return;

  ProcessTimer test("Strategy: Maxing XYZ tree");
  cache->fillPrimary(Constants::DataUnavailable);
  
  const size_t gridZ = cache->getNumLayers();
  const size_t gridY = cache->getNumLats();
  const size_t gridX = cache->getNumLons();
  const auto& haves = db->getHaves();
  const auto& missings = db->getMissings();

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    auto w = output->getFloat2D("weights");
    w->fill(0);
    auto& wa = output->getFloat2DRef("weights");
    auto gridtestP = output->getFloat2D();
    gridtestP->fill(0);
    auto& gridtest = output->getFloat2DRef();

    for (auto it = obsManager.begin(); it != obsManager.end(); ++it) {
      auto r = std::static_pointer_cast<PayloadSourceList<2>>(it->second);
      for (auto& v : r->myObs[z]) {
        const int atX = v.x - offsetX;
        const int atY = v.y - offsetY;
        if ((atX < 0) || (atY < 0) || (atX >= static_cast<int>(gridX)) || (atY >= static_cast<int>(gridY))) {
          continue;
        }
        auto& hit  = wa[atY][atX];
        auto& vref = gridtest[atY][atX];
        const auto rv = v.data[0] / v.data[1];
        
        if (hit > 0) vref = (rv > vref) ? rv : vref;
        else vref = rv;
        
        if (precision > 0) vref = Arith::roundOff(vref, precision);
        hit = 1;
      }
    }
  }

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    auto& wa       = output->getFloat2DRef("weights");
    auto& gridtest = output->getFloat2DRef();
    for (size_t x = 0; x < gridX; x++) {
      for (size_t y = 0; y < gridY; y++) {
        auto& hit = wa[y][x];
        if (hit < 1) {
          auto& vref = gridtest[y][x];
          const size_t globalX = offsetX + x;
          const size_t globalY = offsetY + y;
          if (missings[haves.getIndex3D(globalX, globalY, z)] >= cutoff) {
            vref = Constants::MissingData;
          } else {
            vref = Constants::DataUnavailable;
          }
        }
      }
    }
  }
}

// ============================================================================
// Multi-Doppler Wind Synthesis
// ============================================================================
#if 0
void 
WindSynthesisStrategy::reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> cache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision)
{
  auto& obsManager = db->getObservationManager();
  
  if (obsManager.getPayloadSize() != 5 && obsManager.getPayloadSize() != 0) {
      fLogSevere("WindSynthesisStrategy requires payload size 5 (M11, M22, M12, P1, P2).");
      return;
  }

  ProcessTimer test("Strategy: Wind Synthesis");
  
  // Set up the primary grid for Magnitude (sqrt(U^2 + V^2))
  cache->fillPrimary(Constants::DataUnavailable);
  cache->setString(Constants::ColorMap, "Velocity");
  
  const size_t gridZ = cache->getNumLayers();
  const size_t gridY = cache->getNumLats();
  const size_t gridX = cache->getNumLons();
  const auto& haves = db->getHaves();
  const auto& missings = db->getMissings();

  // Control Flag for Single-Radar Coverage Handling
  // Set to 1 to project 1D radial wind (Option 1). Set to 0 to mask with a sentinel value (Option 2).
  #define PROJECT_1D_WIND 1

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    
    // U and V secondary grids
    auto U_ptr = output->addFloat2D("U", "m/s", {0, 1});
    auto V_ptr = output->addFloat2D("V", "m/s", {0, 1});
    
    // Intermediate accumulation grids
    auto M11_ptr = output->addFloat2D("M11", "Dimensionless", {0, 1});
    auto M22_ptr = output->addFloat2D("M22", "Dimensionless", {0, 1});
    auto M12_ptr = output->addFloat2D("M12", "Dimensionless", {0, 1});
    auto P1_ptr  = output->addFloat2D("P1", "Dimensionless", {0, 1});
    auto P2_ptr  = output->addFloat2D("P2", "Dimensionless", {0, 1});

    M11_ptr->fill(0); M22_ptr->fill(0); M12_ptr->fill(0); P1_ptr->fill(0); P2_ptr->fill(0);

    // Hide intermediate grids so they aren't written to the final NetCDF file
    output->setVisible("M11", false);
    output->setVisible("M22", false);
    output->setVisible("M12", false);
    output->setVisible("P1", false);
    output->setVisible("P2", false);
    
    auto& m11 = output->getFloat2DRef("M11");
    auto& m22 = output->getFloat2DRef("M22");
    auto& m12 = output->getFloat2DRef("M12");
    auto& p1  = output->getFloat2DRef("P1");
    auto& p2  = output->getFloat2DRef("P2");

    // 1. Accumulate the 5 components
    for (auto it = obsManager.begin(); it != obsManager.end(); ++it) {
      auto r = std::static_pointer_cast<PayloadSourceList<5>>(it->second);
      for (auto& v : r->myObs[z]) {
        const int atX = v.x - offsetX;
        const int atY = v.y - offsetY;
        if ((atX < 0) || (atY < 0) || (atX >= static_cast<int>(gridX)) || (atY >= static_cast<int>(gridY))) {
          continue;
        }
        m11[atY][atX] += v.data[0];
        m22[atY][atX] += v.data[1];
        m12[atY][atX] += v.data[2];
        p1[atY][atX]  += v.data[3];
        p2[atY][atX]  += v.data[4];
      }
    }
    
    // 2. Solve the 2x2 Matrix for U, V, and Magnitude
    auto& U = output->getFloat2DRef("U");
    auto& V = output->getFloat2DRef("V");
    auto& Mag = output->getFloat2DRef(); 
    
    for (size_t x = 0; x < gridX; x++) {
      for (size_t y = 0; y < gridY; y++) {
          
          float d11 = m11[y][x];
          float d22 = m22[y][x];
          float d12 = m12[y][x];
          
          if (d11 == 0 && d22 == 0) {
              const size_t globalX = offsetX + x;
              const size_t globalY = offsetY + y;
              if (missings[haves.getIndex3D(globalX, globalY, z)] >= cutoff) {
                Mag[y][x] = Constants::MissingData;
              } else {
                Mag[y][x] = Constants::DataUnavailable;
              }
              U[y][x] = Constants::DataUnavailable;
              V[y][x] = Constants::DataUnavailable;
              continue;
          }
          
          // Calculate Determinant
          float det = (d11 * d22) - (d12 * d12);
          
          // Check for single-radar coverage or parallel beams
          if (det < 0.05f) {
              float trace = d11 + d22;

#if PROJECT_1D_WIND
              // Option 1: Project the 1D radial wind directly onto the grid
              float u_val = p1[y][x] / trace;
              float v_val = p2[y][x] / trace;
              U[y][x] = u_val;
              V[y][x] = v_val;
              Mag[y][x] = std::sqrt(u_val * u_val + v_val * v_val);
#else
              // Option 2: Apply a dedicated sentinel value to mark single-radar areas
              // For operations, map this constant to rConstants.cc/h
              const float SingleRadarCoverage = -99904.0f;
              Mag[y][x] = SingleRadarCoverage;
              U[y][x] = SingleRadarCoverage;
              V[y][x] = SingleRadarCoverage;
#endif
              continue;
          }
          
          // Invert matrix and solve (Multi-Doppler)
          float u_val = (d22 * p1[y][x] - d12 * p2[y][x]) / det;
          float v_val = (d11 * p2[y][x] - d12 * p1[y][x]) / det;
          
          U[y][x] = u_val;
          V[y][x] = v_val;
          Mag[y][x] = std::sqrt(u_val * u_val + v_val * v_val);
      }
    }
  }
  
  fLogInfo("{}", test);
}
#endif
void 
WindSynthesisStrategy::reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> cache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision)
{
  auto& obsManager = db->getObservationManager();
  
  if (obsManager.getPayloadSize() != 5 && obsManager.getPayloadSize() != 0) {
      fLogSevere("WindSynthesisStrategy requires payload size 5 (M11, M22, M12, P1, P2).");
      return;
  }

  ProcessTimer test("Strategy: Wind Synthesis");
  
  // Set up the primary grid for Magnitude (sqrt(U^2 + V^2))
  cache->fillPrimary(Constants::DataUnavailable);
  cache->setString(Constants::ColorMap, "Velocity"); 
  
  const size_t gridZ = cache->getNumLayers();
  const size_t gridY = cache->getNumLats();
  const size_t gridX = cache->getNumLons();
  const auto& haves = db->getHaves();
  const auto& missings = db->getMissings();

  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    output->setString(Constants::ColorMap, "Velocity");
    
    // U and V secondary grids (These STAY visible)
    output->addFloat2D("U", "m/s", {0, 1});
    output->addFloat2D("V", "m/s", {0, 1});
    
    // Use the API to allocate intermediate math arrays
    auto M11_ptr = output->addFloat2D("M11", "Dimensionless", {0, 1});
    auto M22_ptr = output->addFloat2D("M22", "Dimensionless", {0, 1});
    auto M12_ptr = output->addFloat2D("M12", "Dimensionless", {0, 1});
    auto P1_ptr  = output->addFloat2D("P1", "Dimensionless", {0, 1});
    auto P2_ptr  = output->addFloat2D("P2", "Dimensionless", {0, 1});

    // Hide them from the NetCDF writer!
    output->setVisible("M11", false);
    output->setVisible("M22", false);
    output->setVisible("M12", false);
    output->setVisible("P1", false);
    output->setVisible("P2", false);

    M11_ptr->fill(0); 
    M22_ptr->fill(0); 
    M12_ptr->fill(0); 
    P1_ptr->fill(0); 
    P2_ptr->fill(0);

    auto& m11 = output->getFloat2DRef("M11");
    auto& m22 = output->getFloat2DRef("M22");
    auto& m12 = output->getFloat2DRef("M12");
    auto& p1  = output->getFloat2DRef("P1");
    auto& p2  = output->getFloat2DRef("P2");

    // 1. Accumulate the 5 components
    for (auto it = obsManager.begin(); it != obsManager.end(); ++it) {
      auto r = std::static_pointer_cast<PayloadSourceList<5>>(it->second);
      for (auto& v : r->myObs[z]) {
        const int atX = v.x - offsetX;
        const int atY = v.y - offsetY;
        if ((atX < 0) || (atY < 0) || (atX >= static_cast<int>(gridX)) || (atY >= static_cast<int>(gridY))) {
          continue;
        }
        m11[atY][atX] += v.data[0];
        m22[atY][atX] += v.data[1];
        m12[atY][atX] += v.data[2];
        p1[atY][atX]  += v.data[3];
        p2[atY][atX]  += v.data[4];
      }
    }
    
    // 2. Solve the 2x2 Matrix for U, V, and Magnitude
    auto& U = output->getFloat2DRef("U");
    auto& V = output->getFloat2DRef("V");
    auto& Mag = output->getFloat2DRef(); 
    
    for (size_t x = 0; x < gridX; x++) {
      for (size_t y = 0; y < gridY; y++) {
          
          float d11 = m11[y][x];
          float d22 = m22[y][x];
          float d12 = m12[y][x];
          
          if (d11 == 0 && d22 == 0) {
              const size_t globalX = offsetX + x;
              const size_t globalY = offsetY + y;
              if (missings[haves.getIndex3D(globalX, globalY, z)] >= cutoff) {
                Mag[y][x] = Constants::MissingData;
              } else {
                Mag[y][x] = Constants::DataUnavailable;
              }
              U[y][x] = Constants::DataUnavailable;
              V[y][x] = Constants::DataUnavailable;
              continue;
          }
          
          float det = (d11 * d22) - (d12 * d12);
          
          // Check for single-radar coverage
          if (det < 0.05f) {
              float trace = d11 + d22;
              if (trace > 0.0001f) {
                  // Project the 1D radial wind directly onto the grid
                  float u_val = p1[y][x] / trace;
                  float v_val = p2[y][x] / trace;
                  U[y][x] = u_val;
                  V[y][x] = v_val;
                  Mag[y][x] = std::sqrt(u_val * u_val + v_val * v_val);
              } else {
                  Mag[y][x] = Constants::DataUnavailable;
                  U[y][x] = Constants::DataUnavailable;
                  V[y][x] = Constants::DataUnavailable;
              }
              continue;
          }
          
          // Invert matrix and solve (Multi-Doppler)
          float u_val = (d22 * p1[y][x] - d12 * p2[y][x]) / det;
          float v_val = (d11 * p2[y][x] - d12 * p1[y][x]) / det;
          
          U[y][x] = u_val;
          V[y][x] = v_val;
          Mag[y][x] = std::sqrt(u_val * u_val + v_val * v_val);
      }
    }
  }
  
  fLogInfo("{}", test);
}
