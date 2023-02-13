#include "rAlgConfigFile.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rIODataType.h"

#include <fstream>

using namespace rapio;
using namespace std;

void
AlgConfigFile::introduce(const string & protocol,
  std::shared_ptr<AlgConfigFile>      factory)
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
    LogSevere("Configuration type for " << path << " unknown. Trying '" << type << "'\n");
  } else {
    type = name.substr(lastdot + 1, name.size());
    Strings::toLower(type);
  }
  auto config = AlgConfigFile::getConfigFile(type);

  if (config == nullptr) {
    LogSevere("Can't find a configuration file reader for '" << type << "'\n");
  }
  return config;
}

std::shared_ptr<AlgConfigFile>
AlgConfigFile::getConfigFile(const std::string& type)
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

void
AlgFLATConfigFile::introduceSelf()
{
  std::shared_ptr<AlgFLATConfigFile> newOne = std::make_shared<AlgFLATConfigFile>();
  Factory<AlgConfigFile>::introduce("config", newOne);
}

bool
AlgXMLConfigFile::readConfigURL(const URL& path,
  std::vector<std::string>               & optionlist,
  std::vector<std::string>               & valuelist)
{
  bool success = false;
  auto conf    = IODataType::read<PTreeData>(path.toString());

  try{
    if (conf != nullptr) {
      auto rootTree = conf->getTree()->getChild("w2algxml");
      auto items    = rootTree.getChildren("option");
      for (auto r: items) {
        const auto option = r.getAttr("letter", std::string(""));
        const auto value  = r.getAttr("value", std::string(""));
        if (!option.empty()) {
          optionlist.push_back(option);
          valuelist.push_back(value);
        }
      }
      success = true;
    }
  }catch (const std::exception& e) {
    LogSevere("Error parsing XML from " << path << ", " << e.what() << "\n");
  }
  return success;
}

bool
AlgXMLConfigFile::writeConfigURL(const URL& path,
  const std::string                       & program,
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
  std::vector<std::string>                & valuelist)
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
  const std::string                        & program,
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
        LogSevere("Couldn't write configuration file to " << filename << " for some reason\n");
        of.close();
        return false;
      }
      buf = of.rdbuf();
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
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
