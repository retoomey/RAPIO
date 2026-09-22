#include "rRandomForest.h"
#include <rError.h>
#include <rStrings.h>
#include <rOS.h>
#include <rIODataType.h>
#include <rPTreeData.h>
#include <rDataTable.h>

#include <fstream>
#include <numeric>
#include <algorithm>

namespace rapio {
// ---------------------------------------------------------------------------
// FeatureVector Implementation
// ---------------------------------------------------------------------------

bool
FeatureVector::set(const std::string& name, double value)
{
  if (myOwner != nullptr) {
    int idx = myOwner->getFeatureIndex(name);
    if (idx != -1) {
      set(static_cast<size_t>(idx), value);
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// RandomForest Implementation
// ---------------------------------------------------------------------------

bool
RandomForest::readForest(const std::string& rfFile)
{
  myTrees.clear();
  myFeatureNameToIndex.clear();
  myFeatureIndexToName.clear();
  myImputationVector.clear();
  myNumberOfTrees = 0;

  std::string ext = OS::getRootFileExtension(rfFile);

  Strings::toLower(ext);

  try {
    if ((ext == "xml") || (ext == "json") ) {
      return xmlToForest(rfFile);
    } else {
      return csvToForest(rfFile);
    }
  } catch (const std::exception& e) {
    fLogSevere("RandomForest read error: {}", e.what());
    return false;
  }
}

bool
RandomForest::readImputation(const std::string& imputationFile)
{
  auto table = IODataType::read<DataTable>(imputationFile, "csv");

  if (!table || (table->getRowCount() == 0)) {
    fLogSevere("Could not open or parse imputation file: {}", imputationFile);
    return false;
  }

  myImputationVector.assign(myFeatureIndexToName.size(), 0.0);
  const auto& colNames = table->getColumnNames();

  for (size_t i = 0; i < myFeatureIndexToName.size(); ++i) {
    const std::string& featName = myFeatureIndexToName[i];
    if (std::find(colNames.begin(), colNames.end(), featName) != colNames.end()) {
      try {
        myImputationVector[i] = table->getColumn(featName).getCellAsFloat(0);
      } catch (const std::exception& e) {
        fLogSevere("Failed to parse imputation value for feature '{}': {}", featName, e.what());
      }
    } else {
      fLogSevere("Imputation file missing column for required feature '{}'", featName);
    }
  }

  fLogInfo("Loaded imputation defaults for {} features from {}", myFeatureIndexToName.size(), imputationFile);
  return true;
}

int
RandomForest::getOrRegisterFeature(const std::string& name)
{
  auto it = myFeatureNameToIndex.find(name);

  if (it != myFeatureNameToIndex.end()) { return it->second; }

  int newIndex = static_cast<int>(myFeatureIndexToName.size());

  myFeatureNameToIndex[name] = newIndex;
  myFeatureIndexToName.push_back(name);
  return newIndex;
}

int
RandomForest::getFeatureIndex(const std::string& name) const
{
  auto it = myFeatureNameToIndex.find(name);

  return (it != myFeatureNameToIndex.end()) ? it->second : -1;
}

const std::string&
RandomForest::getFeatureName(size_t index) const
{
  static const std::string emptyString = "";

  if (index < myFeatureIndexToName.size()) {
    return myFeatureIndexToName[index];
  }
  return emptyString;
}

bool
RandomForest::xmlToForest(const std::string& rfFile)
{
  auto ptreeData = IODataType::read<PTreeData>(rfFile, "xml");

  if (!ptreeData) {
    fLogSevere("RandomForest: Failed to read XML file via PTreeData: {}", rfFile);
    return false;
  }

  auto root = ptreeData->getTree();

  if (!root) {
    fLogSevere("RandomForest: Root PTreeNode is null in {}", rfFile);
    return false;
  }

  auto dataNode = root->getChildOptional("table.data");
  std::vector<PTreeNode> items;

  if (dataNode) {
    items = dataNode->getChildren("item");
  } else {
    items = root->getChildren("item");
  }

  if (items.empty()) {
    fLogSevere("RandomForest: No <item> elements found in XML: {}", rfFile);
    return false;
  }

  std::vector<TreeNode> currentTree;
  int currentTreeNum = -1;

  for (const auto& item : items) {
    TreeNode node;
    int treeNum         = item.getAttr("tableNumber", -1);
    std::string varName = item.getAttr("featureName", std::string(""));

    node.leftChild   = item.getAttr("leftChild", -1);
    node.rightChild  = item.getAttr("rightChild", -1);
    node.threshold   = item.getAttr("threshold", 0.0);
    node.probability = item.getAttr("probability", 0.0);

    node.isLeaf       = (varName == "leaf" || varName.empty());
    node.featureIndex = node.isLeaf ? -1 : getOrRegisterFeature(varName);

    if (treeNum != currentTreeNum) {
      if (!currentTree.empty()) {
        myTrees.push_back(currentTree);
      }
      currentTree.clear();
      currentTreeNum = treeNum;
    }
    currentTree.push_back(node);
  }

  if (!currentTree.empty()) {
    myTrees.push_back(currentTree);
  }

  myNumberOfTrees = static_cast<int>(myTrees.size());
  fLogInfo("Initialized Random Forest (XML) with {} trees from {}", myNumberOfTrees, rfFile);
  return true;
} // RandomForest::xmlToForest

bool
RandomForest::csvToForest(const std::string& rfFile)
{
  auto table = IODataType::read<DataTable>(rfFile, "csv");

  if (!table) {
    fLogSevere("Could not open or parse RandomForest CSV file: {}", rfFile);
    return false;
  }

  size_t rowCount = table->getRowCount();

  if (rowCount == 0) {
    fLogSevere("RandomForest CSV file is empty: {}", rfFile);
    return false;
  }

  try {
    auto& treeNumCol = table->getColumn("tableNumber");
    auto& featureCol = table->getColumn("featureName");
    auto& leftCol    = table->getColumn("leftChild");
    auto& rightCol   = table->getColumn("rightChild");
    auto& threshCol  = table->getColumn("threshold");
    auto& probCol    = table->getColumn("probability");

    std::vector<TreeNode> currentTree;
    int currentTreeNum = -1;

    for (size_t r = 0; r < rowCount; ++r) {
      TreeNode node;

      int treeNum         = treeNumCol.getCellAsInt(r);
      std::string varName = featureCol.getCellAsString(r);

      node.leftChild    = leftCol.getCellAsInt(r);
      node.rightChild   = rightCol.getCellAsInt(r);
      node.threshold    = threshCol.getCellAsFloat(r);
      node.probability  = probCol.getCellAsFloat(r);
      node.isLeaf       = (varName == "leaf" || varName.empty());
      node.featureIndex = node.isLeaf ? -1 : getOrRegisterFeature(varName);

      if (treeNum != currentTreeNum) {
        if (!currentTree.empty()) {
          myTrees.push_back(currentTree);
        }
        currentTree.clear();
        currentTreeNum = treeNum;
      }
      currentTree.push_back(node);
    }

    if (!currentTree.empty()) {
      myTrees.push_back(currentTree);
    }

    myNumberOfTrees = static_cast<int>(myTrees.size());
    fLogInfo("Initialized Random Forest (CSV) with {} trees from {}", myNumberOfTrees, rfFile);
    return true;
  } catch (const std::exception& e) {
    fLogSevere("RandomForest CSV is missing required columns or has a type mismatch: {}", e.what());
    return false;
  }
} // RandomForest::csvToForest

ForestProbability
RandomForest::getForestProbability(const FeatureVector& fv) const
{
  ForestProbability result;

  // 1. Safety Gate Check: Ensure at least 50% of predictors were populated
  if (!fv.hasSufficientData(myMinDataRatio)) {
    fLogSevere("Over 50% of predictors missing ({}/{} set). Assigning 0.0 probability.",
      fv.getSetCount(), fv.size());
    return result;
  }

  // 2. Build local evaluation vector: Use extracted observation if set, else imputation default
  std::vector<double> evalVector(fv.size());

  for (size_t i = 0; i < fv.size(); ++i) {
    if (fv.isSet(i)) {
      evalVector[i] = fv.myValues[i];
    } else if (i < myImputationVector.size()) {
      evalVector[i] = myImputationVector[i];
    } else {
      evalVector[i] = 0.0;
    }
  }

  // 3. Fast O(1) Decision Tree Traversal and Feature Ranking
  double addedProbability = 0.0;
  std::map<std::string, double> totalContribs;

  for (const auto& tree : myTrees) {
    int nodeIdx = 0;
    int prevIdx = 0;

    while (!tree[nodeIdx].isLeaf) {
      const TreeNode& node = tree[nodeIdx];
      prevIdx = nodeIdx;

      nodeIdx = (evalVector[node.featureIndex] > node.threshold) ?
        node.rightChild : node.leftChild;

      totalContribs[myFeatureIndexToName[node.featureIndex]] +=
        (tree[prevIdx].probability - tree[nodeIdx].probability);
    }
    addedProbability += tree[nodeIdx].probability;
  }

  int divisor = std::max<int>(1, myNumberOfTrees);

  result.probability = addedProbability / divisor;

  for (auto& pair : totalContribs) {
    result.predictorContributions[pair.first] = pair.second / divisor;
    result.rankedFractions.push_back({ pair.first, std::abs(pair.second) });
  }

  std::sort(result.rankedFractions.begin(), result.rankedFractions.end(),
    [](const auto& a, const auto& b) {
      return a.second > b.second;
    });

  return result;
} // RandomForest::getForestProbability

ForestProbability
RandomForest::getForestProbability(
  const std::map<std::string, double>& attributes,
  const std::map<std::string, double>& imputationValues,
  int *                              missingCount) const
{
  FeatureVector fv = createFeatureVector();
  int missing      = 0;

  for (size_t i = 0; i < myFeatureIndexToName.size(); ++i) {
    const std::string& fName = myFeatureIndexToName[i];
    auto attrIt = attributes.find(fName);

    if ((attrIt != attributes.end()) && Constants::isGood(attrIt->second)) {
      fv.set(i, attrIt->second);
    } else {
      missing++;
    }
  }

  if (missingCount) { *missingCount = missing; }

  // If internal imputation vector is not yet loaded, temporarily backfill from imputationValues map
  if (myImputationVector.empty() && !imputationValues.empty()) {
    const_cast<RandomForest *>(this)->readImputation(""); // Initialize vector
    for (size_t i = 0; i < myFeatureIndexToName.size(); ++i) {
      const std::string& fName = myFeatureIndexToName[i];
      auto impIt = imputationValues.find(fName);
      if (impIt != imputationValues.end()) {
        const_cast<RandomForest *>(this)->myImputationVector[i] = impIt->second;
      }
    }
  }

  return getForestProbability(fv);
}
} // namespace rapio
