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
  LogInfo("wgrib2 module: \"" << wgrib2api_info() << "\"\n");
}

std::vector<std::string>
IOWgrib::capture_vstdout_of_wgrib2(int argc, const char * argv[])
{
  std::vector<std::string> voutput;

  bool useCapture = true;

  if (useCapture) {
    int pipefd[2];

    pipe(pipefd); // pipefd[0] = read, pipefd[1] = write

    // Save original stdout
    int stdout_copy = dup(STDOUT_FILENO);

    // Redirect stdout to pipe
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]); // We don't need the write end anymore

    // Run wgrib2()
    wgrib2(argc, argv);

    // Restore stdout
    fflush(stdout);
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);

    std::string partial_line;
    char buffer[4096];

    ssize_t count;

    while ((count = ::read(pipefd[0], buffer, sizeof(buffer))) > 0) {
      size_t start = 0;
      for (ssize_t i = 0; i < count; ++i) {
        if (buffer[i] == '\n') {
          // Append current segment to form a complete line (without '\n')
          partial_line.append(&buffer[start], i - start);
          voutput.push_back(std::move(partial_line));
          partial_line.clear();
          start = i + 1;
        }
      }

      // Append any remaining partial content
      if (start < count) {
        partial_line.append(&buffer[start], count - start);
      }
    }

    // Add final line if not newline-terminated
    if (!partial_line.empty()) {
      voutput.push_back(std::move(partial_line));
    }

    close(pipefd[0]);
  } else {
    // Run wgrib2()
    wgrib2(argc, argv);
  }

  return voutput;
} // IOWgrib::capture_vstdout_of_wgrib2

std::shared_ptr<DataType>
IOWgrib::readGribDataType(const URL& url)
{
  #if 0
  // Lazy read the url if exists, handle later with custom calls.
  // FIXME: still working on flushing out routines.
  std::shared_ptr<WgribDataTypeImp> g = std::make_shared<WgribDataTypeImp>(url);
  return g; // Our new gribtype

  #endif

  #if 1
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

  LogInfo("wgrib2 reader: " << url << "\n");

  // virtual to static, we only handle file/url
  return (IOWgrib::readGribDataType(URL(params)));
}

bool
IOWgrib::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & params
)
{
  LogSevere("Called wgrib2 write module correctly.  Oh yay!\n");
  return false;
}

IOWgrib::~IOWgrib(){ }
