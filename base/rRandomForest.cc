#include "rRandomForest.h"
#include <rError.h>
#include <rStrings.h>
#include <rOS.h>
#include <rIODataType.h>
#include <rPTreeData.h>
#include <fstream>
#include <numeric>
#include <algorithm>

namespace rapio {
bool
RandomForest::readForest(const std::string& rfFile)
{
  myTrees.clear();
  myFeatureNameToIndex.clear();
  myFeatureIndexToName.clear();
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
  std::ifstream file(rfFile);

  if (!file.is_open()) {
    fLogSevere("Could not open RandomForest CSV file: {}", rfFile);
    return false;
  }

  std::string headerLine;

  std::getline(file, headerLine);
  if (!headerLine.empty() && (headerLine.back() == '\r') ) { headerLine.pop_back(); }

  std::vector<std::string> headers;

  Strings::splitWithoutEnds(headerLine, ',', &headers);

  std::vector<TreeNode> currentTree;
  int currentTreeNum = -1;

  std::string line;

  while (std::getline(file, line)) {
    if (!line.empty() && (line.back() == '\r') ) { line.pop_back(); }
    std::vector<std::string> tokens;
    Strings::splitWithoutEnds(line, ',', &tokens);

    if (tokens.size() != headers.size()) { continue; }

    TreeNode node;
    int treeNum = -1;
    std::string varName;

    for (size_t i = 0; i < tokens.size(); ++i) {
      if (headers[i] == "tableNumber") { treeNum = std::stoi(tokens[i]); } else if (headers[i] == "featureName") {
        varName = tokens[i];
      } else if (headers[i] == "leftChild") {
        node.leftChild = std::stoi(tokens[i]);
      } else if (headers[i] == "rightChild") {
        node.rightChild = std::stoi(tokens[i]);
      } else if (headers[i] == "threshold") {
        node.threshold = std::stod(tokens[i]);
      } else if (headers[i] == "probability") { node.probability = std::stod(tokens[i]); }
    }

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
} // RandomForest::csvToForest

ForestProbability
RandomForest::getForestProbability(
  const std::map<std::string, double>& attributes,
  const std::map<std::string, double>& imputationValues,
  int *                              missingCount) const
{
  if (missingCount) { *missingCount = 0; }
  double addedProbability = 0.0;

  std::vector<double> fastFeatures(myFeatureIndexToName.size(), Constants::MissingData);

  for (size_t i = 0; i < myFeatureIndexToName.size(); ++i) {
    const std::string& fName = myFeatureIndexToName[i];
    auto attrIt = attributes.find(fName);
    double val  = (attrIt != attributes.end()) ? attrIt->second : Constants::MissingData;

    if (val == Constants::MissingData) {
      auto impIt = imputationValues.find(fName);
      val = (impIt != imputationValues.end()) ? impIt->second : 0.0;
      if (missingCount) { *missingCount += 1; }
    }
    fastFeatures[i] = val;
  }

  std::map<std::string, double> totalContribs;

  for (const auto& tree : myTrees) {
    int nodeIdx = 0;
    int prevIdx = 0;

    while (!tree[nodeIdx].isLeaf) {
      const TreeNode& node = tree[nodeIdx];
      prevIdx = nodeIdx;

      nodeIdx = (fastFeatures[node.featureIndex] > node.threshold) ?
        node.rightChild : node.leftChild;

      totalContribs[myFeatureIndexToName[node.featureIndex]] +=
        (tree[prevIdx].probability - tree[nodeIdx].probability);
    }
    addedProbability += tree[nodeIdx].probability;
  }

  ForestProbability result;
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
} // namespace rapio
