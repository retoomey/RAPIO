#include "rIOPython.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"

// FIXME: I might play with direct python mapping
// later vs our hybrid method
// pkg-config -cflags python
//#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
//#include "Python.h"
//#include "numpy/arrayobject.h"
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
    auto dataptr = datagrid->getFloat2D(Constants::PrimaryDataName);
    if (dataptr == nullptr) {
      LogSevere("Python call only allowed on 2D LatLonGrid and RadialSet at moment :(\n");
      return std::vector<std::string>();
    }
    auto x       = theDims[0].size();
    auto y       = theDims[1].size();
    auto size    = x * y;                // actual data
    auto memsize = size * sizeof(float); // a lot bigger.  FIXME: This depends on type of data!
    auto& ref2   = dataptr->ref();
    // ref2[0][0] = 999.0;
    // ref2[1][1] = -99000.0;

    std::string key = std::to_string(pid) + "-array" + std::to_string(1); // FIXME: loop
    arrayNames.push_back(key);
    shared_memory_object shdmem { open_or_create, key.c_str(), read_write };
    shdmem.truncate(memsize);
    // shdmem.get_name()
    // shdmem.get_size()
    mapped_region region2 { shdmem, read_write }; // read only, read_write?
    float * at = static_cast<float *>(region2.get_address());
    memcpy(at, ref2.data(), size * sizeof(float));

    // ----------------------------------------------------
    // Call the python helper.
    OS::runProcess(command, output);

    // Do we need to copy back?  Aren't we mapped to this?
    memcpy(ref2.data(), at, size * sizeof(float));
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
}

void
IOPython::handleCommandParam(const std::string& command,
   std::map<std::string, std::string> &outputParams)
{
  // The default is factory=outputfolder.  Python for example splits
  // the command param into script,outputfolder
  std::vector<std::string> pieces;
  Strings::splitWithoutEnds(command, ',', &pieces);
  if (pieces.size() < 2){
    LogSevere("PYTHON= format should be scriptpath,outputfolder\n");
    LogSevere("        Tried to parse from '"<<command<<"'\n");
  }
  outputParams["scriptname"] = pieces[0];
  outputParams["outputfolder"] = pieces[1];
}

bool
IOPython::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
  // -------------------------------------------------------------------
  // Settings
  bool outputPython = (keys["print"] == "true");
  const std::string pythonScript = keys["scriptname"];
  const std::string outputFolder = keys["outputfolder"];
  std::string  filename = keys["filename"];
  if (filename.empty()){
    LogSevere("Need a filename to output\n");
    return false;
  }

  // Try a first time hunt for python...assuming which installed
  // FIXME: Could be OS routine..
  static bool havePython = false;
  static std::string python = "/usr/bin/python";
  if (!havePython){
    // Try to find python...because of python2, 3, etc. This isn't clear-cut
    // which needs to be installed
    std::vector<std::string> pythonnames = {"python", "python2", "python3"};
    for (auto p:pythonnames){
      std::vector<std::string> pythonWhich;
      OS::runProcess("which "+p, pythonWhich);
      for(auto o:pythonWhich){
       if (o[0] == '/'){
         python = o;
         havePython = true;
         break;
       }
      }
      if (havePython){ break; }
    }
    havePython = true;
  }
  auto p = keys["bin"]; // force override the python with setting.  Check for it?
  if (!p.empty()){ python = p; }
  // -------------------------------------------------------------------

  // FIXME: Ok at the moment only DataGrid supported, though I can see
  // expanding this to be general DataType
  bool success = false;
  auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (dataGrid != nullptr){
    std::string pythonCommand = python+" "+pythonScript;
    LogInfo("RUN PYTHON:" << pythonCommand << " BASEURL: " << filename << "\n");
    auto output = runDataProcess(pythonCommand, filename, dataGrid);

    // Hunt python output for RAPIO tags
    bool haveFileBack = false;
    bool haveFactoryBack = false;
    for (auto v:output) {
      // We'll always use RAPIO to mark returns.  I want it to fail as soon as possible for speed
      if ((v.size() > 4) && (v[0] == 'R') && (v[1] == 'A') && (v[2] == 'P') && (v[3] == 'I') && (v[4] == 'O')){
          if (Strings::removePrefix(v, "RAPIO_FILE_OUT:")){
             keys["filename"] = v;
             haveFileBack = true;
             continue;
          }else if (Strings::removePrefix(v, "RAPIO_FACTORY_OUT:")){
             keys["factory"] = v;
             haveFactoryBack = true;
             continue;
          }
      }
      // Output lines from python if turned on
      if (outputPython){
        LogInfo("PYTHON:"<<v<<"\n");
      }
    }
    success = haveFileBack && haveFactoryBack;

  }else{
    LogSevere("This is not a DataGrid or subclass, can't call Python yet with this\n");
  }

  return success;
} // IOPython::encodeDataType
