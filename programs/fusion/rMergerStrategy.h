#pragma once

#include <rRAPIOOptions.h>
#include <rLLHGridN2D.h>
#include "rFusionDatabase.h" 
#include <memory>
#include <string>
#include <vector>

namespace rapio {

// ============================================================================
// 1. The Strategy Interface
// ============================================================================
class MergerStrategy {
public:
  MergerStrategy(const std::string& name) : myName(name) {}
  virtual ~MergerStrategy() = default;

  std::string getName() const { return myName; }

  // Tells rFusion2 what kind of output grids this strategy generates
  // (e.g., Wind generates "U" and "V", Average generates "Reflectivity")
  virtual void declareOutputGrids(std::vector<std::string>& gridNames) = 0;

  // The core MapReduce "Reduce" step.
  // Receives the database pointer and executes the specific mathematical reduction
  // directly into the outputCache.
  virtual void reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> outputCache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision) = 0;

protected:
  std::string myName;
};

// ============================================================================
// 2. Concrete Strategy: Weighted Average (Legacy mergeTo)
// ============================================================================
class WeightedAverageStrategy : public MergerStrategy {
public:
  WeightedAverageStrategy() : MergerStrategy("Average") {}

  virtual void declareOutputGrids(std::vector<std::string>& gridNames) override {
    gridNames.push_back("Average");
  }

  virtual void reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> outputCache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision) override;
};

// ============================================================================
// 3. Concrete Strategy: Maximum Value (Legacy maxTo)
// ============================================================================
class MaximumValueStrategy : public MergerStrategy {
public:
  MaximumValueStrategy() : MergerStrategy("Max") {}

  virtual void declareOutputGrids(std::vector<std::string>& gridNames) override {
    gridNames.push_back("Max");
  }

  virtual void reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> outputCache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision) override;
};

// ============================================================================
// 4. Concrete Strategy: Multi-Doppler Wind Synthesis
// ============================================================================
class WindSynthesisStrategy : public MergerStrategy {
public:
  WindSynthesisStrategy() : MergerStrategy("Wind") {}

  virtual void declareOutputGrids(std::vector<std::string>& gridNames) override {
    gridNames.push_back("U_Wind");
    gridNames.push_back("V_Wind");
  }

  virtual void reduce(
    FusionDatabase* db,
    std::shared_ptr<LLHGridN2D> outputCache,
    const time_t cutoff,
    size_t offsetX, size_t offsetY,
    float precision) override;
};

} // namespace rapio
