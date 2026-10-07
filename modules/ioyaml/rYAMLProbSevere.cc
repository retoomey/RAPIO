#include "rYAMLProbSevere.h"

using namespace rapio;

void
YAMLProbSevere::introduceSelf(IOYAML * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<YAMLProbSevere>();

  // Register to look for the explicit "ProbSevere" identifier inside the JSON
  owner->introduce("ProbSevere", io);
}

bool
YAMLProbSevere::canHandle(std::shared_ptr<PTreeData> tree)
{
  if (!tree || !tree->getTree()) { return false; }

  std::string typeId = tree->getTree()->get<std::string>("product", "");

  return typeId.find("ProbSevere") != std::string::npos;
}

std::shared_ptr<DataType>
YAMLProbSevere::downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in)
{
  auto jsonTree = std::dynamic_pointer_cast<PTreeData>(in);

  if (!jsonTree) { return nullptr; }

  return std::make_shared<ProbSevere>(jsonTree);
}
