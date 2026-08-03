#include "rIOPython.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"

// FIXME: I might play with direct python mapping
// later vs our hybrid method
// pkg-config -cflags python
// #define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
// #include "Python.h"
// #include "numpy/arrayobject.h"
#include "rOS.h"

#include <rBOOST.h>
BOOST_WRAP_PUSH
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>
BOOST_WRAP_POP

#include <algorithm>

using namespace boost::interprocess;

// Cleaner, but will test with all our compiler versions
namespace fs = boost::filesystem;
using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOPython();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

namespace {
// These put all the memory moves in one place and make them safe enough
// to keep semgrep happy.

// Helper to safely copy data TO a shared memory region
bool
safeCopyToRegion(boost::interprocess::mapped_region& region, const void * src, size_t size)
{
  if (region.get_size() < size) {
    fLogSevere("Shared memory region too small for copy! Expected {} but got {}", size, region.get_size());
    return false;
  }
  auto * dst_char = static_cast<char *>(region.get_address());
  auto * src_char = static_cast<const char *>(src);

  std::copy(src_char, src_char + size, dst_char);
  return true;
}

// Helper to safely copy data FROM a shared memory region
bool
safeCopyFromRegion(const boost::interprocess::mapped_region& region, void * dst, size_t size)
{
  if (region.get_size() < size) {
    fLogSevere("Shared memory region too small for extraction! Expected {} but got {}", size, region.get_size());
    return false;
  }
  auto * src_char = static_cast<const char *>(region.get_address());
  auto * dst_char = static_cast<char *>(dst);

  std::copy(src_char, src_char + size, dst_char);
  return true;
}
}

std::string
IOPython::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that allows sending data to a python script for filtering or output.";
  return help;
}

void
IOPython::initialize()
{ }

IOPython::~IOPython()
{ }

std::shared_ptr<DataType>
IOPython::createDataType(IOConfig& config)
{
  fLogSevere("Python scripts cannot currently create DataTypes.");
  return nullptr;
}

std::vector<std::string>
IOPython::runDataProcess(const std::string& command,
  const std::string& filename, std::shared_ptr<DataGrid> datagrid)
{
  // FIXME: can we create boost arrays as shared to begin with?
  // and thus avoid copies? Maybe
  auto pid = OS::getProcessID();

  std::string jsonName = std::to_string(pid) + "-JSON";
  std::vector<std::string> arrayNames;
  std::vector<std::string> output;

  try{
    // ----------------------------------------------------
    // Write JSON out to shared for data process/python
    // We're not allowing write to attributes at moment.  FIXME?
    std::shared_ptr<PTreeData> theJson = datagrid->createMetadata();

    // Add extra stuff, like path information.  Writing Python should use
    // this path and information for any written files.  It should be
    // hidden in the PYTHON API
    auto root = theJson->getTree();
    PTreeNode fileinfo;
    fileinfo.put("filebase", filename);
    root->addNode("RAPIOOutput", fileinfo);

    std::vector<char> buf; // FIXME: Buffer class instead?
    IOConfig keys;
    size_t aLength = IODataType::writeBuffer(theJson, buf, keys, "json");
    if (aLength < 2) { // Check for empty buffer (buffer always ends with 0)
      fLogSevere("DataGrid didn't generate JSON so aborting python call.");
      return std::vector<std::string>();
    }

    aLength -= 1; // Remove the ending buffer 0
    shared_memory_object shdmem2 { open_or_create, jsonName.c_str(), read_write };
    shdmem2.truncate(aLength);
    mapped_region region3 { shdmem2, read_write }; // read only, read_write?
    if (!safeCopyToRegion(region3, buf.data(), aLength)) {
      // Throw so we can cleanup the memory
      throw std::runtime_error("Failed to copy JSON to shared memory.");
    }

    // ----------------------------------------------------
    // Write the arrays to shared memory
    // FIXME: We write the primary data array for now, if any.
    // FIXME: generalize by looping and handle the data TYPE such as float, int
    auto theDims = datagrid->getDims();


    // -------------------------------------------------------
    // START ARRAYS
    //
    // OK we gotta make it super generic to make it work
    // for(size_t i=0; i< theDims.size(); i++){
    //  fLogSevere("DIM {} size is {}", i, theDims[i].size());
    // }
    auto list = datagrid->getArrays();
    size_t count = 0;
    std::vector<void *> in, out;
    std::vector<size_t> moveSizes;

    // Shared memories to use per array
    std::vector<shared_memory_object> memory;

    for (auto l:list) {
      // This gets the total count of the data before multiplying by the storage type
      // FIXME: function
      auto ddims       = l->getDimIndexes(); // std::vector<size_t>
      const size_t s   = ddims.size();
      size_t totalSize = 0;
      if (s > 0) {
        totalSize = theDims[ddims[0]].size();
        if (s > 1) {
          for (size_t i = 1; i < s; ++i) {
            totalSize *= theDims[ddims[i]].size(); // Multiply dimensions
          }
        }
      }
      // Total byte size from array.
      // FIXME: function
      size_t totalBytes = totalSize;
      auto theType      = l->getStorageType();
      if (theType == FLOAT) {
        totalBytes = totalSize * sizeof(float);
      } else if (theType == INT) {
        totalBytes = totalSize * sizeof(int);
      } else {
        fLogSevere("Declaring unknown type.");
      }

      // Save array file key list
      std::string key = std::to_string(pid) + "-array" + std::to_string(count + 1);
      count++;
      arrayNames.push_back(key);

      // Create shared memory with unique name matching process and array so
      // we don't step on other RAPIO programs
      memory.push_back({ open_or_create, key.c_str(), read_write });
      auto& m = memory[memory.size() - 1];
      m.truncate(totalBytes);
      // m.get_name()
      // m.get_size()
      mapped_region region { m, read_write };
      auto ref = l->getRawDataPointer();
      if (!safeCopyToRegion(region, ref, totalBytes)) {
        throw std::runtime_error("Failed to copy array to shared memory.");
      }
      in.push_back(ref); // should be ok to hold the pointer here, synchronous
      moveSizes.push_back(totalBytes);
    }
    // END ARRAYS
    // -------------------------------------------------------

    // ----------------------------------------------------
    // Call the python helper.
    OS::runProcess(command, output);

    // Copy back RAM to RAM, since our array isn't shared
    // to begin with. Note we could maybe have python flag so
    // we only do this if things were changed.  RAM copies
    // are still pretty fast
    size_t pushCount = 0;
    for (auto l:list) {
      mapped_region region { memory[pushCount], read_only };
      if (!safeCopyFromRegion(region, in[pushCount], moveSizes[pushCount])) {
        throw std::runtime_error("Failed to copy array to shared memory.");
      }
      pushCount++;
    }
  }catch (const std::exception& e) {
    fLogSevere("Failed to execute command {}", command);
  }

  // Clean up all shared memory objects...
  // The boost object not supporting RAII?
  shared_memory_object::remove(jsonName.c_str());
  for (size_t i = 0; i < arrayNames.size(); i++) {
    std::string key = std::to_string(pid) + "-array" + std::to_string(i + 1);
    shared_memory_object::remove(key.c_str());
  }
  return output;
} // IOPython::runDataProcess

void
IOPython::handleCommandParam(const std::string& command,
  IOConfig                                    &outputParams)
{
  // The default is factory=outputfolder.  Python for example splits
  // the command param into script,outputfolder
  std::vector<std::string> pieces;

  Strings::splitWithoutEnds(command, ',', &pieces);
  auto s = pieces.size();

  outputParams.set("scriptname", (s > 0) ? pieces[0] : "");
  outputParams.set("outputfolder", (s > 1) ? pieces[1] : "./");
  if (s < 2) {
    fLogSevere("PYTHON= format should be scriptpath,outputfolder");
    fLogSevere("        Tried to parse from '{}'", command);
  }
}

bool
IOPython::encodeDataType(std::shared_ptr<DataType> dt,
  IOConfig                                         & keys
)
{
  // -------------------------------------------------------------------
  // Settings
  bool outputPython = (keys.get("print") == "true");
  const std::string pythonScript = keys.get("scriptname");
  const std::string outputFolder = keys.get("outputfolder");
  std::string filename = keys.get("filename");

  if (filename.empty()) {
    fLogSevere("Need a filename to output");
    return false;
  }

  // Try a first time hunt for python
  // This code could also be in OS maybe.  Given a list of relative
  // or absolute paths, find a working exe
  static bool huntedPython  = false;
  static std::string python = "/usr/bin/pythonfail";

  if (!huntedPython) {
    huntedPython = true;
    std::vector<std::string> pythonnames = { "python", "python2", "python3" };
    const auto search = OS::findValidExe(pythonnames);
    if (!search.empty()) {
      python = search;
    }
  }

  auto p = keys.get("bin"); // force override the python with setting.  Check for it?

  if (!p.empty()) { python = p; }
  // -------------------------------------------------------------------

  // FIXME: Ok at the moment only DataGrid supported, though I can see
  // expanding this to be general DataType
  bool success  = false;
  auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);

  if (dataGrid != nullptr) {
    std::string pythonCommand = python + " " + pythonScript;
    fLogInfo("RUN PYTHON: {} BASEURL: {}", pythonCommand, filename);
    auto output = runDataProcess(pythonCommand, filename, dataGrid);

    // Hunt python output for RAPIO tags
    bool haveFileBack    = false;
    bool haveFactoryBack = false;
    for (auto v:output) {
      // We'll always use RAPIO to mark returns.  I want it to fail as soon as possible for speed
      if ((v.size() > 4) && (v[0] == 'R') && (v[1] == 'A') && (v[2] == 'P') && (v[3] == 'I') && (v[4] == 'O')) {
        if (Strings::removePrefix(v, "RAPIO_FILE_OUT:")) {
          keys.set("filename", v);
          haveFileBack = true;
          continue;
        } else if (Strings::removePrefix(v, "RAPIO_FACTORY_OUT:")) {
          keys.set("factory", v);
          haveFactoryBack = true;
          continue;
        }
      }
      // Output lines from python if turned on
      if (outputPython) {
        fLogInfo("PYTHON: {}", v);
      }
    }
    success = haveFileBack && haveFactoryBack;
    #if 0
    if (!haveFileBack) {
      fLogSevere("Your python needs to print RAPIO_FILE_OUT: filename");
    }
    if (!haveFactoryBack) {
      fLogSevere("Your python needs to print RAPIO_FACTORY_OUT: factory");
    }
    #endif
  } else {
    fLogSevere("This is not a DataGrid or subclass, can't call Python yet with this");
  }

  return success;
} // IOPython::encodeDataType
