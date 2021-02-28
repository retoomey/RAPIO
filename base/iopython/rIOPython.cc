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
    auto dataptr = datagrid->getFloat2D("primary");
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
    output = OS::runProcess(command);

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

bool
IOPython::encodeDataType(std::shared_ptr<DataType> dt,
  const std::string                               & params,
  std::shared_ptr<PTreeNode>                      dfs,
  bool                                            directFile,
  // Output for notifiers
  std::vector<Record>                             & records
)
{
  // Param format is scriptpath,outputfolder
  // unlike others that are typically just outputfolder
  std::vector<std::string> pieces;
  Strings::splitWithoutEnds(params, ',', &pieces);
  if (pieces.size() < 2){
    LogSevere("PYTHON= format should be scriptpath,outputfolder\n");
    LogSevere("        Tried to parse from '"<<params<<"'\n");
    return false;
  }
  std::string pythonScript = pieces[0];
  std::string outputFolder = pieces[1];

  // -------------------------------------------------------------------
  // Settings
  static bool havePython = false;
  static std::string python = "/usr/bin/python";

  bool outputPython = false;
  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      python = output.getAttr("bin", python);
      std::string outPython = "";
      outPython = output.getAttr("print", outPython);
      outputPython = (outPython == "true");
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  // Try a first time hunt for python...assuming which installed
  // FIXME: Could be OS routine..
  if (!havePython){
    // Try to find python...because of python2, 3, etc. This isn't clear-cut
    // which needs to be installed
    std::vector<std::string> pythonnames = {"python", "python2", "python3"};
    for (auto p:pythonnames){
      auto pythonWhich = OS::runProcess("which "+p);
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
  // -------------------------------------------------------------------

  // FIXME: Ok at the moment only DataGrid supported, though I can see
  // expanding this to be general DataType
  auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (dataGrid != nullptr){
    //
    bool useSubDirs = true; // Use subdirs
    URL aURL        = IODataType::generateFileName(dt, outputFolder, "", directFile, useSubDirs);

    std::string pythonCommand = python+" "+pythonScript;
    LogInfo("RUN PYTHON:" << pythonCommand << "\n");
    auto output = runDataProcess(pythonCommand, aURL.toString(), dataGrid);

    // Hunt python output for RAPIO tags
    std::string fileout, factory;
    for (auto v:output) {
      // We'll always use RAPIO to mark returns.  I want it to fail as soon as possible for speed
      if ((v.size() > 4) && (v[0] == 'R') && (v[1] == 'A') && (v[2] == 'P') && (v[3] == 'I') && (v[4] == 'O')){
          if (Strings::removePrefix(v, "RAPIO_FILE_OUT:")){
             fileout = v;
             continue;
          }else if (Strings::removePrefix(v, "RAPIO_FACTORY_OUT:")){
             factory = v;
             continue;
          }
      }
      // Output lines from python if turned on
      if (outputPython){
        LogInfo("PYTHON:"<<v<<"\n");
      }
    }

    // Make record using output from python...
    if (fileout != ""){
      IODataType::generateRecord(dt, URL(fileout), factory, records);
    }
  }else{
    LogSevere("This is not a DataGrid or subclass, can't call Python yet with this\n");
  }

  return false;
} // IOPython::encodeDataType
