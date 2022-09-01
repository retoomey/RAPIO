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
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

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
IOPython::createDataType(const std::string& params)
{
  LogSevere("Python scripts cannot currently create DataTypes.\n");
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
    size_t aLength = IODataType::writeBuffer(theJson, buf, "json");
    if (aLength < 2) { // Check for empty buffer (buffer always ends with 0)
      LogSevere("DataGrid didn't generate JSON so aborting python call.\n");
      return std::vector<std::string>();
    }

    aLength -= 1; // Remove the ending buffer 0
    shared_memory_object shdmem2 { open_or_create, jsonName.c_str(), read_write };
    shdmem2.truncate(aLength);
    mapped_region region3 { shdmem2, read_write }; // read only, read_write?
    char * at2 = static_cast<char *>(region3.get_address());
    memcpy(at2, &buf[0], aLength);

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
    //  LogSevere("DIM " << i << " size is " << theDims[i].size() << "\n");
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
            totalSize += theDims[ddims[i]].size();
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
        LogSevere("Declaring unknown type.\n");
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

      auto * at = region.get_address();
      auto ref  = l->getRawDataPointer();
      memcpy(at, ref, totalBytes); // At least it's a ram to ram copy
      in.push_back(ref);           // should be ok to hold the pointer here, synchronous
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
      auto * at = region.get_address();
      memcpy(in[pushCount], at, moveSizes[pushCount]); // At least it's a ram to ram copy
      pushCount++;
    }
  }catch (const std::exception& e) {
    LogSevere("Failed to execute command " << command << "\n");
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
  std::map<std::string, std::string>          &outputParams)
{
  // The default is factory=outputfolder.  Python for example splits
  // the command param into script,outputfolder
  std::vector<std::string> pieces;

  Strings::splitWithoutEnds(command, ',', &pieces);
  auto s = pieces.size();

  outputParams["scriptname"]   = (s > 0) ? pieces[0] : "";
  outputParams["outputfolder"] = (s > 1) ? pieces[1] : "./";
  if (s < 2) {
    LogSevere("PYTHON= format should be scriptpath,outputfolder\n");
    LogSevere("        Tried to parse from '" << command << "'\n");
  }
}

bool
IOPython::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>               & keys
)
{
  // -------------------------------------------------------------------
  // Settings
  bool outputPython = (keys["print"] == "true");
  const std::string pythonScript = keys["scriptname"];
  const std::string outputFolder = keys["outputfolder"];
  std::string filename = keys["filename"];

  if (filename.empty()) {
    LogSevere("Need a filename to output\n");
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

  auto p = keys["bin"]; // force override the python with setting.  Check for it?

  if (!p.empty()) { python = p; }
  // -------------------------------------------------------------------

  // FIXME: Ok at the moment only DataGrid supported, though I can see
  // expanding this to be general DataType
  bool success  = false;
  auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);

  if (dataGrid != nullptr) {
    std::string pythonCommand = python + " " + pythonScript;
    LogInfo("RUN PYTHON:" << pythonCommand << " BASEURL: " << filename << "\n");
    auto output = runDataProcess(pythonCommand, filename, dataGrid);

    // Hunt python output for RAPIO tags
    bool haveFileBack    = false;
    bool haveFactoryBack = false;
    for (auto v:output) {
      // We'll always use RAPIO to mark returns.  I want it to fail as soon as possible for speed
      if ((v.size() > 4) && (v[0] == 'R') && (v[1] == 'A') && (v[2] == 'P') && (v[3] == 'I') && (v[4] == 'O')) {
        if (Strings::removePrefix(v, "RAPIO_FILE_OUT:")) {
          keys["filename"] = v;
          haveFileBack     = true;
          continue;
        } else if (Strings::removePrefix(v, "RAPIO_FACTORY_OUT:")) {
          keys["factory"] = v;
          haveFactoryBack = true;
          continue;
        }
      }
      // Output lines from python if turned on
      if (outputPython) {
        LogInfo("PYTHON:" << v << "\n");
      }
    }
    success = haveFileBack && haveFactoryBack;
    #if 0
    if (!haveFileBack) {
      LogSevere("Your python needs to print RAPIO_FILE_OUT: filename\n");
    }
    if (!haveFactoryBack) {
      LogSevere("Your python needs to print RAPIO_FACTORY_OUT: factory\n");
    }
    #endif
  } else {
    LogSevere("This is not a DataGrid or subclass, can't call Python yet with this\n");
  }

  return success;
} // IOPython::encodeDataType
