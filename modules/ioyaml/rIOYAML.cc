#include "rIOYAML.h"
#include <rPTreeData.h>
#include <rError.h>
#include <rIOURL.h>
#include <rStrings.h>

// Default built in DataType support
#include "rYAMLProbSevere.h"

// Include the RapidYAML single header
// We 'might' use the single cpp as well, will have
// to check compile times, etc.
#define RYML_SINGLE_HDR_DEFINE_NOW
#include "rapidyaml.h"

#include <fstream>

using namespace rapio;

extern "C" {
void *
createRAPIOIO(void)
{
  auto * z = new IOYAML();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
}

// Using sniffers to look for fields that match a known DataType
using PTreeSnifferFunc = bool (*)(const std::shared_ptr<rapio::PTreeData>&, std::string&);

// ---------------------------------------------------------
// Format Identifiers (Sniffers)
// ---------------------------------------------------------

bool
checkProbSevere(const std::shared_ptr<rapio::PTreeData>& yaml, std::string& outKey)
{
  auto root = yaml->getTree();

  if (!root) { return false; }

  std::string typeId = root->get<std::string>("product", "");

  if (typeId.find("ProbSevere") != std::string::npos) {
    outKey = "ProbSevere";
    return true;
  }
  return false;
}

std::string
IOYAML::getHelpString(const std::string& key)
{
  return "builder for reading/writing YAML, JSON, and GeoJSON formatted data using RapidYAML.";
}

void
IOYAML::initialize()
{
  // Override RapidYAML's default error handler so it throws C++ exceptions
  // instead of calling std::abort(). This prevents bad files from crashing RAPIO.
  ryml::Callbacks cb = ryml::get_callbacks();

  cb.m_error_basic = [](ryml::csubstr msg, ryml::ErrorDataBasic const&, void *) {
      throw std::runtime_error(std::string(msg.str ? msg.str : "", msg.len));
    };
  cb.m_error_parse = [](ryml::csubstr msg, ryml::ErrorDataParse const&, void *) {
      throw std::runtime_error(std::string(msg.str ? msg.str : "", msg.len));
    };
  cb.m_error_visit = [](ryml::csubstr msg, ryml::ErrorDataVisit const&, void *) {
      throw std::runtime_error(std::string(msg.str ? msg.str : "", msg.len));
    };

  ryml::set_callbacks(cb);

  YAMLProbSevere::introduceSelf(this);
}

// Recursive helper to map RapidYAML nodes directly into Boost Property Tree
static void
rymlToPtree(ryml::NodeRef node, boost::property_tree::ptree& pt)
{
  if (node.is_map()) {
    for (ryml::NodeRef child : node.children()) {
      boost::property_tree::ptree child_pt;
      rymlToPtree(child, child_pt);
      // ryml keys are string views; convert to std::string for ptree
      std::string key(child.key().data(), child.key().size());
      pt.push_back(std::make_pair(key, child_pt));
    }
  } else if (node.is_seq()) {
    for (ryml::NodeRef child : node.children()) {
      boost::property_tree::ptree child_pt;
      rymlToPtree(child, child_pt);

      // Changed from "" to "item" so Boost generates valid XML tags
      // instead of <> when writing arrays to an XML file.
      pt.push_back(std::make_pair("item", child_pt));
    }
  } else if (node.has_val()) {
    std::string val(node.val().data(), node.val().size());
    pt.put_value(val);
  }
}

// Recursive helper to map Boost Property Tree into RapidYAML (Modern API)
static void
ptreeToRyml(const boost::property_tree::ptree& pt, ryml::NodeRef node)
{
  if (pt.empty()) {
    // Safely copy the string into the YAML tree's memory arena and set the value.
    std::string val = pt.get_value<std::string>();
    node.set_val(node.tree()->to_arena(val));
  } else {
    // Check if this node is an array/sequence.
    bool isSeq = true;
    for (const auto& child : pt) {
      if (!child.first.empty() && (child.first != "item")) {
        isSeq = false;
        break;
      }
    }

    // RapidYAML version quirk: operator|= is marked deprecated, but set_type()
    // hasn't been added yet. We locally suppress the false-positive warning.
    #if defined(__GNUC__) || defined(__clang__)
    # pragma GCC diagnostic push
    # pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif

    if (isSeq) {
      node |= ryml::SEQ;
      for (const auto& child : pt) {
        ptreeToRyml(child.second, node.append_child());
      }
    } else {
      node |= ryml::MAP;
      for (const auto& child : pt) {
        auto child_node = node.append_child();
        // Safely copy the key into the arena and set it
        child_node.set_key(child_node.tree()->to_arena(child.first));
        ptreeToRyml(child.second, child_node);
      }
    }

    #if defined(__GNUC__) || defined(__clang__)
    # pragma GCC diagnostic pop
    #endif
  }
} // ptreeToRyml

#if 0
std::shared_ptr<PTreeData>
IOYAML::readPTreeDataBuffer(std::vector<char>& buffer)
{
  try {
    if (buffer.empty()) { return nullptr; }

    // ryml requires a mutable buffer for in-situ parsing.
    // We create a csubstr from the vector data.
    ryml::csubstr yaml_str(buffer.data(), buffer.size());
    ryml::Tree tree = ryml::parse_in_place(ryml::to_substr(buffer));

    std::shared_ptr<PTreeData> d = std::make_shared<PTreeData>();
    auto& n = d->getTree()->node;

    rymlToPtree(tree.rootref(), n);
    return d;
  } catch (const std::exception& e) {
    fLogSevere("Exception reading YAML/JSON data... {} ignoring", e.what());
  }
  return nullptr;
}

#endif // if 0
std::shared_ptr<PTreeData>
IOYAML::readPTreeDataBuffer(std::vector<char>& buffer)
{
  try {
    if (buffer.empty()) { return nullptr; }

    // Strip any trailing null terminators added by encoders or tests
    size_t len = buffer.size();
    while (len > 0 && buffer[len - 1] == '\0') {
      len--;
    }

    if (len == 0) { return nullptr; }

    // ryml requires a mutable buffer for in-situ parsing.
    // Create a substr using the corrected length
    ryml::substr yaml_str(buffer.data(), len);
    ryml::Tree tree = ryml::parse_in_place(yaml_str);

    std::shared_ptr<PTreeData> d = std::make_shared<PTreeData>();
    auto& n = d->getTree()->node;

    rymlToPtree(tree.rootref(), n);
    return d;
  } catch (const std::exception& e) {
    fLogSevere("Exception reading YAML/JSON data... {} ignoring", e.what());
  }
  return nullptr;
}

std::shared_ptr<DataType>
IOYAML::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  return readPTreeDataBuffer(buffer);
}

#if 0
std::shared_ptr<DataType>
IOYAML::createDataType(IOConfig& config)
{
  const URL url(config.getParamURL());

  fLogInfo("YAML reader: {}", url.toString());
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<PTreeData> yaml = readPTreeDataBuffer(buf);
    if (yaml) {
      // 1. Identify JSON payload type (based on ProbSevere format conventions)
      std::string typeId = yaml->getTree()->get<std::string>("product", "");

      // 2. See if we have a registered handler for this type
      if (!typeId.empty()) {
        auto fmt  = getIOSpecializer(typeId);
        auto pFmt = std::dynamic_pointer_cast<PTreeDataSpecializer>(fmt);
        if (pFmt) {
          return pFmt->downcastPTreeDataType(config, yaml);
        }
      }
      return yaml;
    }
  }
  fLogSevere("Unable to create YAML/JSON from {}", url.toString());
  return nullptr;
}

#endif // if 0

std::shared_ptr<DataType>
IOYAML::createDataType(IOConfig& config)
{
  const URL url(config.getParamURL());

  fLogInfo("YAML reader: {}", url.toString());
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<PTreeData> yaml = readPTreeDataBuffer(buf);
    if (yaml) {
      // 1. Ask all registered specializers if they recognize this payload
      for (const auto& pair : mySpecializers) {
        auto pFmt = std::dynamic_pointer_cast<PTreeDataSpecializer>(pair.second);
        if (pFmt && pFmt->canHandle(yaml)) {
          fLogInfo("Found a YAML specializer that can handle it");
          return pFmt->downcastPTreeDataType(config, yaml);
        }
      }

      // 2. Fallback: If no specializer claimed it, check if it explicitly defines a DataType
      std::string typeId = yaml->getTree()->get<std::string>("DataType", "");
      if (!typeId.empty()) {
        auto fmt  = getIOSpecializer(typeId);
        auto pFmt = std::dynamic_pointer_cast<PTreeDataSpecializer>(fmt);
        if (pFmt) {
          return pFmt->downcastPTreeDataType(config, yaml);
        }
      }

      // 3. Return generic PTreeData if it's unrecognized
      return yaml;
    }
  }
  fLogSevere("Unable to create YAML/JSON from {}", url.toString());
  return nullptr;
} // IOYAML::createDataType

#if 0
size_t
IOYAML::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer, IOConfig& config)
{
  std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);

  if (!ptree) {
    fLogSevere("YAML encoder requires a PTreeData object.");
    return 0;
  }

  try {
    ryml::Tree tree;
    ryml::NodeRef root = tree.rootref();

    // Convert the Boost tree to the RapidYAML tree
    ptreeToRyml(ptree->getTree()->node, root);

    // Emit the YAML to a string
    std::string out = ryml::emitrs_yaml<std::string>(tree);

    // Copy to the output buffer
    buffer.assign(out.begin(), out.end());
    buffer.push_back('\0'); // Always nice to null terminate memory buffers

    return buffer.size();
  } catch (const std::exception& e) {
    fLogSevere("Error encoding YAML buffer: {}", e.what());
  }
  return 0;
}

#endif // if 0

// A lightweight auto-formatter for minified JSON
static std::string
prettifyJSON(const std::string& minified)
{
  std::string out;
  int indent    = 0;
  bool inQuotes = false;

  for (size_t i = 0; i < minified.length(); ++i) {
    char c = minified[i];

    // Track if we are inside a string literal to ignore structural characters
    if ((c == '"') && ((i == 0) || (minified[i - 1] != '\\'))) {
      inQuotes = !inQuotes;
    }

    if (inQuotes) {
      out += c;
      continue;
    }

    if ((c == '{') || (c == '[')) {
      out    += c;
      out    += '\n';
      indent += 4;
      out.append(indent, ' ');
    } else if ((c == '}') || (c == ']')) {
      out    += '\n';
      indent -= 4;
      out.append(indent, ' ');
      out += c;
    } else if (c == ',') {
      out += c;
      out += '\n';
      out.append(indent, ' ');
    } else if (c == ':') {
      out += ": ";
    } else {
      out += c;
    }
  }
  return out;
} // prettifyJSON

size_t
IOYAML::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer, IOConfig& config)
{
  std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);

  if (!ptree) {
    fLogSevere("YAML/JSON encoder requires a PTreeData object.");
    return 0;
  }

  try {
    ryml::Tree tree;
    ryml::NodeRef root = tree.rootref();

    // Convert the Boost tree to the RapidYAML tree
    ptreeToRyml(ptree->getTree()->node, root);

    // Determine the format based on the suffix
    std::string suffix = config.get("suffix");
    Strings::toLower(suffix);

    std::string out;
    if ((suffix == "json") || (suffix == "geojson")) {
      std::string minified = ryml::emitrs_json<std::string>(tree);
      // If they want it pretty (indent=true is stored in IOConfig by default if requested)
      if (config.get("indent") == "true") {
        out = prettifyJSON(minified);
      } else {
        out = minified;
      }
    } else {
      out = ryml::emitrs_yaml<std::string>(tree);
    }

    // Copy to the output buffer
    buffer.assign(out.begin(), out.end());
    // Add a trailing newline for clean terminal output
    buffer.push_back('\n');
    buffer.push_back('\0');

    return buffer.size();
  } catch (const std::exception& e) {
    fLogSevere("Error encoding YAML/JSON buffer: {}", e.what());
  }
  return 0;
} // IOYAML::encodeDataTypeBuffer

#if 0
bool
IOYAML::encodeDataType(std::shared_ptr<DataType> dt, IOConfig& keys)
{
  std::string filename;

  // Resolve the final filename, defaulting to .yaml
  if (!resolveFileName(keys, "yaml", "yaml-", filename)) {
    return false;
  }

  bool successful = false;

  try {
    std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);
    if (ptree != nullptr) {
      ptree->preWrite(keys);

      std::vector<char> buffer;
      if (encodeDataTypeBuffer(dt, buffer, keys) > 0) {
        std::ofstream out(filename, std::ios::out | std::ios::binary);
        if (out.is_open()) {
          // Write without the trailing null character we added
          out.write(buffer.data(), buffer.size() - 1);
          out.close();
          successful = true;
        } else {
          fLogSevere("Couldn't open {} for writing.", filename);
        }
      }
      ptree->postWrite(keys);
    }
  } catch (const std::exception& e) {
    fLogSevere("YAML create error: {} {}", filename, e.what());
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }
  if (successful) {
    showFileInfo("YAML writer: ", keys);
  }

  return successful;
} // IOYAML::encodeDataType

#endif // if 0
bool
IOYAML::encodeDataType(std::shared_ptr<DataType> dt, IOConfig& keys)
{
  // Grab the suffix, default to yaml if empty
  std::string suffix = keys.get("suffix");

  if (suffix.empty()) {
    suffix = "yaml";
  }
  Strings::toLower(suffix);

  std::string filename;

  // Resolve the final filename using the dynamic suffix
  if (!resolveFileName(keys, suffix, suffix + "-", filename)) {
    return false;
  }

  bool successful = false;

  try {
    std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);
    if (ptree != nullptr) {
      ptree->preWrite(keys);

      std::vector<char> buffer;
      if (encodeDataTypeBuffer(dt, buffer, keys) > 0) {
        std::ofstream out(filename, std::ios::out | std::ios::binary);
        if (out.is_open()) {
          out.write(buffer.data(), buffer.size() - 1);
          out.close();
          successful = true;
        } else {
          fLogSevere("Couldn't open {} for writing.", filename);
        }
      }
      ptree->postWrite(keys);
    }
  } catch (const std::exception& e) {
    fLogSevere("YAML/JSON create error: {} {}", filename, e.what());
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }
  if (successful) {
    std::string extra;
    if ((suffix == "json") || (suffix == "geojson")) {
      bool indented = (keys.get("indent") == "true");
      extra = fmt::format(" (mode: {} indent: {})", suffix, indented ? "true" : "false");
    } else {
      extra = fmt::format(" (mode: {})", suffix);
    }

    showFileInfo("YAML writer: ", keys, extra);
  }

  return successful;
} // IOYAML::encodeDataType
