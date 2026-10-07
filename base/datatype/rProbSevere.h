#pragma once
#include <rPTreeData.h>
#include <rLL.h>
#include <vector>

namespace rapio {
// Lightweight struct to hold individual storm features
struct ProbSevereFeature {
  int             id = -1;
  float           probSevereVal = 0.0f;
  std::vector<LL> polygon;
};

class ProbSevere : public PTreeData {
public:
  /** One constructor to handle both brand-new objects and promotions */
  explicit
  ProbSevere(std::shared_ptr<PTreeData> genericTree = nullptr);

  /** Destroy us */
  virtual
  ~ProbSevere() = default;

  /** Parse the features from tree */
  void
  parseFeatures();

  /** Returns the parsed version (e.g., "3.0"), or "1.0" if
   * unspecified */
  std::string
  getVersion() const { return myVersion; }

  /** Get the features for an algorithm, the algorithm queries
   * this instead of traversing PTreeNodes directly */
  const std::vector<ProbSevereFeature>&
  getFeatures() const { return myFeatures; }

protected:
  std::vector<ProbSevereFeature> myFeatures;
  std::string myVersion;
};
}
