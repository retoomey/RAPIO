#include "rIOWgrib.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rConfig.h"
#include "rConfigIODataType.h"
#include "rLatLonGrid.h"

#include <fstream>
#include "patch/f_rapio_callback.h"

using namespace rapio;

extern "C" {
#include <wgrib2.h>
}

namespace rapio {
static std::shared_ptr<LatLonGrid> myTempLatLonGrid;

/** Root virtual class visitor for wgrib2 match callbacks.
 *
 * Note: These are called by wgrib c source, so using
 * Log routines will basically infinite loop. We
 * capture the output of wgrib2 when calling.
 * If you need to print debugging I would do something
 * like std::cout << "\nmymessage\n".  You'll basically
 * break into the curreng wgrib2 output
 */
class WGRIB2Callback : public Utility {
public:
  virtual
  ~WGRIB2Callback() = default;

  /** Initialize at the start of a grib2 catalog pass */
  virtual void
  handleInitialize() = 0;

  /** Finalize at the end of a grib2 catalog pass */
  virtual void
  handleFinalize() = 0;

  /** Fill a LLCoverageArea wanted for grid interpolation, if we handle grids. */
  virtual void
  handleGetLLCoverageArea(double * nwLat, double * nwLon, double * seLat, double * seLon, double * dLat, double * dLon,
    int * nLat, int * nLon) = 0;

  /** Handle getting data from wgrib2 */
  virtual void
  handleData(const float * data, int n) = 0;
};

/** Callback handling a grib2 data field */
class FirstWGRIB2Callback : public WGRIB2Callback {
public:

  /** Initialize at the start of a grib2 catalog pass */
  void
  handleInitialize() override
  {
    std::cout << "\n[C++] Initialize\n";
  }

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize()
  {
    std::cout << "\n[C++] Finalize\n";
  }

  /** Our C++ get coverage area.  Used for wgrib2 and output */
  LLCoverageArea
  getLLCoverageArea()
  {
    // FIXME: We can get from param, pass down.  Or maybe
    // the grib suggests a grid and we can modify if we want.
    // FIXME: Bug in LLCoverageArea actually.  LatLonGrid, etc. use the nw corner,
    // the delta and the count.  It doesn't actually use the se corner.
    #if 0
    double lat_start = 60.0;
    double lon_start = -150.0;
    double dlat      = 0.05;
    double dlon      = 0.05;
    int nlat         = 1000;
    int nlon         = 2000;
    #endif
    // double lat_start = 70.0;
    double lat_start = 90.0;
    double lon_start = -220.0;
    double dlat      = 0.05;
    double dlon      = 0.05;
    int nlat         = 2000;
    int nlon         = 4300;

    // Calculate.  LLCoverageArea should do this actually (have two methods,
    // passing sizes and passing coordinates and autosizing.)
    double lat_end = lat_start - (dlat * nlat * 1.0);
    double lon_end = lon_start + (dlon * nlon * 1.0);

    // X and Y are confusing.  X is marching in lon, Y is marching in lat.
    LLCoverageArea a(lat_start, lon_start, lat_end, lon_end, dlat, dlon, nlon, nlat);
    return a;
  }

  /** Get a LLCoverageArea wanted for grid interpolation. */
  void
  handleGetLLCoverageArea(double * nwLat, double * nwLon, double * seLat, double * seLon, double * dLat, double * dLon,
    int * nLat, int * nLon)
  {
    // Send coverage area to C
    LLCoverageArea a = getLLCoverageArea();

    *nwLat = a.getNWLat();
    *nwLon = a.getNWLon();
    *seLat = a.getSELat();
    *seLon = a.getSELon();
    *dLat  = a.getLatSpacing();
    *dLon  = a.getLonSpacing();
    *nLon  = a.getNumX(); // related to X  FIXME: API naming could be better
    *nLat  = a.getNumY(); // related to Y
  }

  /** Handle getting data from wgrib2 */
  void
  handleData(const float * data, int n)
  {
    // Since this is called by wgrib2, any output get captured
    // by our wrapper, thus we don't call Log here but std::cout
    std::cout << "\n[C++] Got " << n << " values from GRIB2\n";
    std::cout << "[C++] ";
    for (size_t i = 0; i < 10; ++i) {
      std::cout << data[i];
      if (i < 10 - 1) { std::cout << ","; }
    }
    std::cout << "\n";

    LLCoverageArea a = getLLCoverageArea();

    myTempLatLonGrid = LatLonGrid::Create(
      "Temperature",
      "K",
      Time(),
      a);

    auto& array = myTempLatLonGrid->getFloat2DRef();

    // wgrib2 should at least fill our array directly. We shouldn't
    // have to transform y or anything here.
    // There are way too many arrays copying and projection and
    // indexing, etc.  But baby steps get it working first.
    // FIXME: Pass the array.data() point into the C.  It's a boost
    // array, but the memory should be correct.
    // void* getFloatArrayPtr(){ return array.data(); or something
    std::memcpy(array.data(), data, n * sizeof(float));
    std::cout << "-----> Done Setting LatLonGrid \n";
  }
};

/** C++ version of the C RapioCallback.
 * This acts as a bridge to a 'true' C++ class. Ok a struct is really just
 * a private class in C++, but...
 * We make this extra one in order a avoid having to create
 * the linker lambas in every subclass, avoid duplication.
 */
struct RAPIOCallbackCPP : public RapioCallback {
  std::shared_ptr<WGRIB2Callback> myCallback;

  RAPIOCallbackCPP(std::shared_ptr<WGRIB2Callback> m) : myCallback(m)
  {
    // Create a vtable for the C calls, linking to the true callback.
    initialize = [](RapioCallback * self) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleInitialize();
      };

    finalize = [](RapioCallback * self) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleFinalize();
      };

    getllcoverage = [](RapioCallback * self, double * nwLat, double * nwLon, double * seLat, double * seLon,
      double * dLat, double * dLon, int * nLat, int * nLon) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleGetLLCoverageArea(nwLat, nwLon, seLat, seLon, dLat,
          dLon, nLat, nLon);
      };

    on_data = [](RapioCallback * self, const float * data, int n) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleData(data, n);
      };
  }
};
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

// We have OS:runProcess, but it captures an actual external process.
// Here wgrib is a function printing to std::cout that we want to capture
// so we can do things with it.
namespace {

std::vector<std::string>
capture_vstdout_of_wgrib2(int argc, const char * argv[])
{
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

  std::vector<std::string> voutput;
  std::string partial_line;
  char buffer[4096];

  ssize_t count;

  while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
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

  return voutput;
} // capture_vstdout_of_wgrib2

}

std::shared_ptr<DataType>
IOWgrib::readGribDataType(const URL& url)
{
  // Create a callback action for our match.
  // FIXME: Just realized we lazy do the grib object, right. So eventually we'll just
  // return a class holding information, then the API calls will trigger these.
  std::shared_ptr<FirstWGRIB2Callback> action = std::make_shared<FirstWGRIB2Callback>();
  RAPIOCallbackCPP test(action);

  std::string filename = url.toString(); // hold in scope
  std::ostringstream oss;

  oss << reinterpret_cast<uintptr_t>(&test); // convert pointer to integer and stream to string
  std::string pointer = oss.str();           // string now holds a decimal address

  // Create wgrib2 parameters, call API
  // We pass parameters to our custom callback function.
  // FIXME: These need to be passed to this module somehow
  const char * argv[] = {
    "wgrib2",
    filename.c_str(), // in scope
    // "-match", ":TMP:850 mb:", // Note: Don't add quotes they will be part of the match
    "-match", ":TMP:2 m above ground:", // Ground should match the us map if projection good.
    // "-match", ":TMP:", // Tell wgrib2 what to match.  Note, no quotes here.
    "-rapio",       // Use our callback patch
    pointer.c_str() // Send address of our RapioCallback class.
  };
  int argc = sizeof(argv) / sizeof(argv[0]);
  std::ostringstream param;

  for (size_t i = 0; i < argc; ++i) {
    param << argv[i];
    if (i < argc - 1) { param << " "; }
  }
  LogInfo("Calling: " << param.str() << "\n");

  std::vector<std::string> result = capture_vstdout_of_wgrib2(argc, argv);

  for (size_t i = 0; i < result.size(); ++i) {
    LogInfo("[wgrib2] " << result[i] << "\n");
  }
  // NOTE: for alpha we're returning a LatLonGrid.  We actually need to
  // return our GribDataType, then have the methods do this on
  // request.  More work to do we're just getting started here.
  if (myTempLatLonGrid) {
    LogInfo("We have a LatLonGrid DataType.\n");
  } else {
    LogInfo("No LatLonGrid found\n");
  }
  return myTempLatLonGrid; // caller will hold it
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
  std::map<std::string, std::string>             & params
)
{
  LogSevere("Called wgrib2 write module correctly.  Oh yay!\n");
  return false;
}

IOWgrib::~IOWgrib(){ }
