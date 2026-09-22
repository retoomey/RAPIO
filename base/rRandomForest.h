#pragma once
#include <map>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <rConstants.h>

namespace rapio {
class RandomForest;

/**
 * @class FeatureVector
 * @brief Helper container representing a single vector of features.
 *
 * Tracks feature values alongside a bitmask of set features, allowing the Random Forest
 * to evaluate completeness and substitute missing features with historical baseline imputation values.
 * @author Robert Toomey
 */
class FeatureVector {
  friend class RandomForest;
public:
  FeatureVector() = default;

  /**
   * @brief Constructs a FeatureVector with a designated size and optional owner reference.
   * @param numFeatures Total number of features required by the Random Forest model.
   * @param owner Pointer to the parent RandomForest for name-to-index resolution.
   */
  FeatureVector(size_t numFeatures, const RandomForest * owner = nullptr)
    : myValues(numFeatures, 0.0),
    mySetMask(numFeatures, false),
    mySetCount(0),
    myOwner(owner){ }

  /**
   * @brief Directly assigns a feature value at the specified index.
   * @param index Array index corresponding to the feature.
   * @param value Extracted numerical value.
   */
  inline void
  set(size_t index, double value)
  {
    if (index < myValues.size()) {
      myValues[index] = value;
      if (!mySetMask[index]) {
        mySetMask[index] = true;
        mySetCount++;
      }
    }
  }

  /**
   * @brief Assigns a feature value by looking up its feature name.
   * @param name Name of the feature (e.g., "AzShear_max").
   * @param value Extracted numerical value.
   * @return True if the feature name was recognized by the forest, false otherwise.
   */
  bool
  set(const std::string& name, double value);

  /** @brief Returns true if the feature at the specified index was explicitly set. */
  inline bool
  isSet(size_t index) const
  {
    return (index < mySetMask.size()) && mySetMask[index];
  }

  /**
   * @brief Gets the raw feature value if set, or returns a fallback sentinel value if unset.
   * @param index Array index corresponding to the feature.
   * @param fallback Default value returned if the feature is unset (default: Constants::MissingData).
   */
  inline double
  getRawValue(size_t index, double fallback = Constants::MissingData) const
  {
    return isSet(index) ? myValues[index] : fallback;
  }

  /**
   * @brief Checks if enough features have been populated to satisfy data availability rules.
   * @param minPercent Minimum required fraction of set features (default: 0.5 / 50%).
   */
  inline bool
  hasSufficientData(double minPercent = 0.5) const
  {
    if (myValues.empty()) { return false; }
    return (static_cast<double>(mySetCount) / myValues.size()) >= minPercent;
  }

  /** @brief Returns the total feature capacity of the vector. */
  size_t
  size() const { return myValues.size(); }

  /** @brief Returns the total number of explicitly set features. */
  size_t
  getSetCount() const { return mySetCount; }

private:
  std::vector<double> myValues;
  std::vector<bool> mySetMask;
  size_t mySetCount = 0;
  const RandomForest * myOwner = nullptr;
};

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
   * @brief Reads and aligns baseline imputation defaults for missing feature handling.
   * @param imputationFile Path to the CSV containing baseline imputation defaults.
   * @return True if loaded successfully, false otherwise.
   */
  bool
  readImputation(const std::string& imputationFile);

  /** @brief Sets the minimum fraction of set features required to evaluate a prediction (e.g., 0.5 = 50%). */
  void setMinDataRatio(double ratio){ myMinDataRatio = ratio; }

  /** @brief Returns the minimum fraction of set features required. */
  double
  getMinDataRatio() const { return myMinDataRatio; }

  /**
   * @brief Factory method to create a FeatureVector sized and linked to this Random Forest.
   */
  FeatureVector
  createFeatureVector() const
  {
    return FeatureVector(myFeatureIndexToName.size(), this);
  }

  /**
   * @brief Evaluates a FeatureVector against the Random Forest decision trees.
   * @param fv The FeatureVector containing extracted radar features.
   * @return A ForestProbability object containing probability and feature rankings.
   */
  ForestProbability
  getForestProbability(const FeatureVector& fv) const;

  /**
   * @brief Legacy evaluation interface supporting std::map input.
   */
  ForestProbability
  getForestProbability(
    const std::map<std::string, double>& attributes,
    const std::map<std::string, double>& imputationValues,
    int *                              missingCount = nullptr) const;

  /** @brief Returns the feature index for a given name (-1 if not used by model). */
  int
  getFeatureIndex(const std::string& name) const;

  /** @brief Returns the feature name at a given index. */
  const std::string&
  getFeatureName(size_t index) const;

  /** @brief Returns the full list of ordered feature names used by this model. */
  const std::vector<std::string>&
  getFeatureNames() const { return myFeatureIndexToName; }

  /** @brief Returns the total number of unique features required by this model. */
  size_t
  getNumFeatures() const { return myFeatureIndexToName.size(); }

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
  std::vector<double> myImputationVector;
  int myNumberOfTrees   = 0;
  double myMinDataRatio = 0.5; // Default: 50%
};
} // namespace rapio
