#pragma once
#include <map>
#include <string>
#include <vector>
#include <utility>
#include <rConstants.h>

namespace rapio {
/**
 * @struct ForestProbability
 * @brief Encapsulates the prediction result and predictor contributions from the random forest.
 */
struct ForestProbability {
  /** @brief The final calculated probability. */
  double                                       probability = 0.0;

  /** @brief The probability contributions mapped by predictor name. */
  std::map<std::string, double>                predictorContributions;

  /** @brief Ranked fractions of predictor contributions (largest to smallest). */
  std::vector<std::pair<std::string, double> > rankedFractions;
};

/**
 * @struct TreeNode
 * @brief A compact representation of a decision tree node optimized for cache locality.
 */
struct TreeNode {
  int    featureIndex = -1;    /**< Index into the flattened feature array. */
  int    leftChild    = -1;    /**< Index of the left child node in the tree array. */
  int    rightChild   = -1;    /**< Index of the right child node in the tree array. */
  double threshold    = 0.0;   /**< The split threshold for the feature. */
  double probability  = 0.0;   /**< The probability value at this node. */
  bool   isLeaf       = false; /**< True if this node is a terminal leaf. */
};

/**
 * @class RandomForest
 * @brief Core random forest implementation for probability prediction and feature ranking.
 */
class RandomForest {
public:
  /** @brief Default constructor. Initialization is deferred to readForest(). */
  RandomForest()  = default;
  ~RandomForest() = default;

  /**
   * @brief Reads and parses a random forest from a CSV, XML, or JSON file.
   * @param filepath The path to the forest configuration file.
   * @return True if the forest was successfully loaded, false otherwise.
   */
  bool
  readForest(const std::string& filepath);

  /**
   * @brief Computes the forest probability given a set of object attributes.
   * @param attributes Map of predictor names to their computed values.
   * @param imputationValues Map of fallback values for missing predictors.
   * @param missingCount Optional pointer to store the number of missing predictors encountered.
   * @return A ForestProbability object containing the final probability and feature contributions.
   */
  ForestProbability
  getForestProbability(
    const std::map<std::string, double>& attributes,
    const std::map<std::string, double>& imputationValues,
    int *                              missingCount = nullptr) const;

  /** @brief Returns the total number of trees loaded in the forest. */
  int
  getNumberOfTrees() const { return myNumberOfTrees; }

private:
  bool
  csvToForest(const std::string& filepath);
  bool
  xmlToForest(const std::string& filepath);
  int
  getOrRegisterFeature(const std::string& name);

  std::vector<std::vector<TreeNode> > myTrees;
  std::map<std::string, int> myFeatureNameToIndex;
  std::vector<std::string> myFeatureIndexToName;
  int myNumberOfTrees = 0;
};
} // namespace rapio
