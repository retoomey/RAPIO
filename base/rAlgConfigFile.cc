#include "rAlgConfigFile.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rIODataType.h"

#include <fstream>

using namespace rapio;
using namespace std;

void
AlgConfigFile::introduce(std::string_view protocol,
  std::shared_ptr<AlgConfigFile>          factory)
{
  Factory<AlgConfigFile>::introduce(protocol, factory);
}

void
AlgConfigFile::introduceSelf()
{
  static bool initialized = false;

  if (!initialized) {
    AlgXMLConfigFile::introduceSelf();
    AlgFLATConfigFile::introduceSelf();
    initialized = true;
  }
}

std::shared_ptr<AlgConfigFile>
AlgConfigFile::getConfigFileForURL(const URL& path)
{
  // This is the only entry point for RAPIOOptions
  introduceSelf();

  // How to tell what type of config file it is?
  // We'll start by using the file suffix.  We could enhance
  // AlgConfigFile class to 'scan' for a file it recognizes...
  std::string type = ".xml"; // WDSS2 by default
  std::string name = path.toString();
  auto lastdot     = name.find_last_of(".");

  if (lastdot == std::string::npos) {
    fLogSevere("Configuration type for {} unknown. Trying '{}'", path.toString(), type);
  } else {
    type = name.substr(lastdot + 1, name.size());
    Strings::toLower(type);
  }
  auto config = AlgConfigFile::getConfigFile(type);

  if (config == nullptr) {
    fLogSevere("Can't find a configuration file reader for '{}'", type);
  }
  return config;
}

std::shared_ptr<AlgConfigFile>
AlgConfigFile::getConfigFile(std::string_view type)
{
  std::shared_ptr<AlgConfigFile> d = Factory<AlgConfigFile>::get(type);

  return (d);
}

void
AlgXMLConfigFile::introduceSelf()
{
  std::shared_ptr<AlgXMLConfigFile> newOne = std::make_shared<AlgXMLConfigFile>();
  Factory<AlgConfigFile>::introduce("xml", newOne);
  Factory<AlgConfigFile>::introduce("json", newOne);
}

bool
AlgXMLConfigFile::parseOptionsFromNode(const rapio::PTreeNode& algNode,
  std::vector<std::string>                                   & options,
  std::vector<std::string>                                   & values)
{
  auto items = algNode.getChildren("option");

  for (auto r: items) {
    const auto option = r.getAttr("letter", std::string(""));
    const auto value  = r.getAttr("value", std::string(""));
    if (!option.empty()) {
      options.push_back(option);
      values.push_back(value);
    }
  }
  return true;
}

void
AlgFLATConfigFile::introduceSelf()
{
  std::shared_ptr<AlgFLATConfigFile> newOne = std::make_shared<AlgFLATConfigFile>();
  Factory<AlgConfigFile>::introduce("config", newOne);
}

bool
AlgXMLConfigFile::readConfigURL(const URL& path,
  std::vector<std::string>               & optionlist,
  std::vector<std::string>               & valuelist,
  std::shared_ptr<PTreeNode> *           outConfigNode)
{
  bool success = false;
  auto conf    = IODataType::read<PTreeData>(path.toString());

  try {
    if ((conf != nullptr) && (conf->getTree() != nullptr)) {
      auto rootTree = conf->getTree()->getChildOptional("w2algxml");
      if (rootTree != nullptr) {
        success = parseOptionsFromNode(*rootTree, optionlist, valuelist);
        if (outConfigNode != nullptr) {
          *outConfigNode = rootTree;
        }
      } else if (auto w2Root = conf->getTree()->getChildOptional("w2merger")) {
        // Allow legacy <w2merger> files to be passed to -iconfig as structured config
        success = true;
        if (outConfigNode != nullptr) {
          *outConfigNode = w2Root;
        }
      }
    }
  } catch (const std::exception& e) {
    fLogSevere("Error parsing XML from {}, {}", path.toString(), e.what());
  }
  return success;
}

bool
AlgXMLConfigFile::readMultiConfigURL(const URL& path, std::vector<AlgConfigNode>& nodes)
{
  auto conf = IODataType::read<PTreeData>(path.toString());

  if ((conf == nullptr) || (conf->getTree() == nullptr)) { return false; }

  try {
    auto tree     = conf->getTree();
    auto rootTree = tree->getChildOptional("w2algxml");

    if (rootTree != nullptr) {
      // Find all nested <w2algxml> blocks (the child algorithms)
      auto algorithms = rootTree->getChildren("w2algxml");

      for (auto& alg : algorithms) {
        AlgConfigNode node;
        node.name = alg.getAttr("program", std::string(""));

        if (!node.name.empty()) {
          node.library = "librapioalg" + node.name + ".so";
        }

        // Parse the flat <option> tags inside this specific child block
        parseOptionsFromNode(alg, node.options, node.values);

        // Preserve the full XML subtree so plugins (e.g. LayerAverage) can query nested tags
        node.configNode = std::make_shared<PTreeNode>(alg);

        nodes.push_back(std::move(node));
      }
      return true;
    }

    // Fallback: translate legacy <w2merger> XML into AlgConfigNodes
    if (auto w2Root = tree->getChildOptional("w2merger")) {
      auto addOpt = [](AlgConfigNode& n, const std::string& k, const std::string& v) {
          if (!v.empty()) {
            n.options.push_back(k);
            n.values.push_back(v);
          }
        };

      // 1. <layeraverage> -> LayerAverage plugin with the <w2merger> subtree attached
      if (!w2Root->getChildren("layeraverage").empty()) {
        AlgConfigNode node;
        node.name       = "LayerAverage";
        node.library    = "librapioalgLayerAverage.so";
        node.configNode = w2Root;
        nodes.push_back(std::move(node));
      }

      // 2. <hda> (+ optional <hdaextras>) -> HDA plugin, and VIL plugin if 263K/233K present
      if (auto hdaNode = w2Root->getChildOptional("hda")) {
        AlgConfigNode hda;
        hda.name       = "HDA";
        hda.library    = "librapioalgHDA.so";
        hda.configNode = hdaNode;

        if (auto h273 = hdaNode->getChildOptional("height273")) {
          addOpt(hda, "h273name", h273->getAttr("product", std::string("")));
          addOpt(hda, "def273", h273->getAttr("default", std::string("")));
        }
        if (auto h253 = hdaNode->getChildOptional("height253")) {
          addOpt(hda, "h253name", h253->getAttr("product", std::string("")));
          addOpt(hda, "def253", h253->getAttr("default", std::string("")));
        }
        if (auto hTerr = hdaNode->getChildOptional("heightTerrain")) {
          addOpt(hda, "terrainname", hTerr->getAttr("product", std::string("")));
        }
        nodes.push_back(std::move(hda));

        // VIL uses height263 (-10C) and height233 (-40C) from <hda>
        auto h263 = hdaNode->getChildOptional("height263");
        auto h233 = hdaNode->getChildOptional("height233");
        if (h263 || h233) {
          AlgConfigNode vil;
          vil.name       = "VIL";
          vil.library    = "librapioalgVIL.so";
          vil.configNode = hdaNode;
          if (h263) {
            addOpt(vil, "h263name", h263->getAttr("product", std::string("")));
            addOpt(vil, "def263", h263->getAttr("default", std::string("")));
          }
          if (h233) {
            addOpt(vil, "h233name", h233->getAttr("product", std::string("")));
            addOpt(vil, "def233", h233->getAttr("default", std::string("")));
          }
          nodes.push_back(std::move(vil));
        }
      }

      return !nodes.empty();
    }
  } catch (const std::exception& e) {
    fLogSevere("Error parsing multi-algorithm XML from {}, {}", path.toString(), e.what());
  }
  return false;
} // AlgXMLConfigFile::readMultiConfigURL

bool
AlgXMLConfigFile::writeConfigURL(const URL& path,
  std::string_view                        program,
  std::vector<std::string>                & optionlist,
  std::vector<std::string>                & valuelist)
{
  // Create document tree
  auto tree = std::make_shared<PTreeData>();
  auto root = tree->getTree();
  PTreeNode w2algxml;

  w2algxml.putAttr("program", program);

  for (size_t i = 0; i < optionlist.size(); ++i) {
    PTreeNode option;
    option.putAttr("letter", optionlist[i]);
    option.putAttr("value", valuelist[i]);
    w2algxml.addNode("option", option);
  }
  root->addNode("w2algxml", w2algxml);

  // Write document tree
  return (IODataType::write(tree, path.toString()));
} // AlgXMLConfigFile::writeConfigURL

bool
AlgFLATConfigFile::readConfigURL(const URL& path,
  std::vector<std::string>                & optionlist, // deliberately avoiding maps
  std::vector<std::string>                & valuelist,
  std::shared_ptr<PTreeNode>              * /*outConfigNode*/)
{
  // HMET flat configuration file reader
  // 1. Anything after a // is a comment
  // 2. Each non empty line has a option:value pair, spaces optional around them
  std::vector<char> buf;
  bool success = false;

  if (IOURL::read(path, buf) > 0) {
    buf.push_back('\0');
    std::istringstream is(&buf.front());
    std::string s;
    while (getline(is, s, '\n')) {
      /** Kill anything at comment or later and remove left/right spaces */
      auto comment = s.find("//");
      if (comment != std::string::npos) {
        s = s.substr(0, comment);
      }
      Strings::trim(s);

      /** Skip empty lines or space lines */
      if (s.size() == 0) { continue; }

      /** Just assume one option:value.  Keep any secondary : with the value */
      auto found = s.find_first_of(":");
      if (found != std::string::npos) {
        std::string option = s.substr(0, found);
        std::string value  = s.substr(found + 1, s.size());
        Strings::trim(option);
        Strings::trim(value);
        optionlist.push_back(option);
        valuelist.push_back(value);
      }
    }
    success = true;
  }

  return success;
} // AlgFLATConfigFile::readConfigURL

bool
AlgFLATConfigFile::writeConfigURL(const URL& path,
  std::string_view                         program,
  std::vector<std::string>                 & optionlist,
  std::vector<std::string>                 & valuelist)
{
  std::streambuf * buf;
  std::ofstream of;
  bool close = false;

  std::string base = path.getBaseName();

  Strings::toLower(base);
  bool console = base == ".config";

  if (console) {
    buf = std::cout.rdbuf();
  } else {
    if (path.isLocal()) {
      close = true;
      std::string filename = path.toString();
      of.open(filename.c_str());
      if (of.fail()) {
        fLogSevere("Couldn't write configuration file to {} for some reason", filename);
        of.close();
        return false;
      }
      buf = of.rdbuf();
    } else {
      fLogSevere("Can't write to a remote URL at {}", path.toString());
      return false;
    }
  }
  std::ostream out(buf);

  out << "// RAPIO generated configuration file.\n";
  out << "// Format FLAT option:value pairs. (Used by HMET, etc.)\n";
  out << "// Generated by program '" << program << "\n\n";
  for (size_t i = 0; i < optionlist.size(); ++i) {
    out << "  " << optionlist[i] << ":" << valuelist[i] << "\n";
  }

  if (close) {
    of.close();
  }

  return true;
} // AlgFLATConfigFile::writeConfigURL
