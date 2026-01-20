#include "rIOWgrib.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rConfig.h"
#include "rConfigIODataType.h"
#include "rLatLonGrid.h"

#include "rWgribCallback.h"
#include "rWgribDataTypeImp.h"

#include "rArrayCallback.h"
#include "rGridCallback.h"

#include <fstream>

using namespace rapio;

extern "C" {
#include <wgrib2.h>
#include <sys/wait.h>
}

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOWgrib();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOWgrib::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses wgrib2 to read grib data files.";

  return help;
}

void
IOWgrib::initialize()
{
  fLogInfo("wgrib2 module: \"{}\"", wgrib2api_info());
}

std::vector<std::string>
IOWgrib::capture_vstdout_of_wgrib2(bool useCapture, int argc, const char * argv[])
{
  using Wgrib2Ptr = int (*)(int, const char *[]);
  Wgrib2Ptr wgrib2_ptr = wgrib2;

  return OS::runFunction(
    useCapture,
    wgrib2,
    argc,
    argv);
} // IOWgrib::capture_vstdout_of_wgrib2

std::shared_ptr<DataType>
IOWgrib::readGribDataType(const URL& url)
{
  #if 1
  // Lazy read the url if exists, handle later with custom calls.
  // FIXME: still working on flushing out routines.
  std::shared_ptr<WgribDataTypeImp> g = std::make_shared<WgribDataTypeImp>(url);
  return g; // Our new gribtype

  #endif

  #if 0
  // -----------------------------------------
  // Just directly return the test LatLonGrid
  //
  std::shared_ptr<GridCallback> action = std::make_shared<GridCallback>(url, ":TMP:surface:");

  action->execute();
  auto hold = GridCallback::myTempLatLonGrid;

  GridCallback::myTempLatLonGrid = nullptr;
  return hold;

  #endif // if 0

  #if 0
  // -----------------------------------------
  // Just directly return the 2D array
  //
  std::shared_ptr<ArrayCallback> action = std::make_shared<ArrayCallback>(url);

  action->execute();
  return nullptr;

  #endif
  // auto hold = GridCallback::myTempLatLonGrid;

  // GridCallback::myTempLatLonGrid = nullptr;
  // return hold;
} // IOWgrib::readGribDataType

std::shared_ptr<DataType>
IOWgrib::createDataType(const std::string& params)
{
  // FIXME: technically a web URL we could read if we
  // copied it locally first for wgrib2
  URL url(params);

  fLogInfo("wgrib2 reader: {}", url.toString());

  // virtual to static, we only handle file/url
  return (IOWgrib::readGribDataType(URL(params)));
}

bool
IOWgrib::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & params
)
{
  fLogSevere("Called wgrib2 write module correctly.  Oh yay!");
  return false;
}

IOWgrib::~IOWgrib(){ }
