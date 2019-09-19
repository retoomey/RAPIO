#include "rAlgConfigFile.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rIOXML.h"

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
  AlgXMLConfigFile::introduceSelf();
  AlgFLATConfigFile::introduceSelf();
}

std::shared_ptr<AlgConfigFile>
AlgConfigFile::getConfigFileForURL(const URL& path)
{
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
  auto conf = IOXML::readURL(path);

  try{
    if (conf != nullptr) {
      for (auto r: conf->get_child("w2algxml")) {
        if (r.first == "option") {
          const auto option = r.second.get("<xmlattr>.letter", "");
          const auto value  = r.second.get("<xmlattr>.value", "");
          if (!option.empty()) {
            optionlist.push_back(option);
            valuelist.push_back(value);
          }
        }
      }
      return true;
    }
  }catch (std::exception e) {
    LogSevere("Error parsing XML from " << path << "\n");
    return false;
  }
  return false;
}

bool
AlgXMLConfigFile::writeConfigURL(const URL& path,
  const std::string                       & program,
  std::vector<std::string>                & optionlist,
  std::vector<std::string>                & valuelist)
{
  // Create document tree
  boost::property_tree::ptree tree;
  tree.put("w2algxml.<xmlattr>.program", program);
  for (size_t i = 0; i < optionlist.size(); ++i) {
    boost::property_tree::ptree option;
    option.add("<xmlattr>.letter", optionlist[i]);
    option.add("<xmlattr>.value", valuelist[i]);
    tree.add_child("w2algxml.option", option);
  }

  // Write document tree
  return (IOXML::writeURL(path, tree, true));
} // AlgXMLConfigFile::writeConfigURL

bool
AlgFLATConfigFile::readConfigURL(const URL& path,
  std::vector<std::string>                & optionlist, // deliberately avoiding maps
  std::vector<std::string>                & valuelist)
{
  // HMET flat configuration file reader
  // 1. Anything after a // is a comment
  // 2. Each non empty line has a option:value pair, spaces optional around them
  Buffer buf;

  if (IOURL::read(path, buf) > 0) {
    buf.data().push_back('\0');
    std::istringstream is(&buf.data().front());
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
    return true;
  }

  return false;
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
