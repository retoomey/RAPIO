#include "rGetColorMap.h"

using namespace rapio;

void
GetColorMap::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Generate SVG legend for a given filename");
  o.setHeader(""); // turn off for first pass
  o.setExample("test.netcdf // ncdump style");

  // Text only treat like a single file...
  o.setTextOnlyMacro("-i file=%s -o=/tmp");
}

void
GetColorMap::processOptions(RAPIOOptions& o)
{
  // FIXME: We'll add more controls/params over time.  We need
  // to also sync with webserver probably.
}

void
GetColorMap::processNewData(rapio::RAPIOData& d)
{
  // The datafile should be 'cached' by linux for multiple calls,
  // so not too worried here...though if too slow we may have to
  // do some work.
  auto data = d.datatype<rapio::DataType>();

  if (data == nullptr) {
    fLogSevere("Failed to get valid DataType");
    return;
  }
  // Read the color map for data, then generate SVG.
  // Do we write to file or return stream?
  // We should be able to directly stream it back I think.
  if (isMacroApplied()) {
    auto c = data->getColorMap();

    std::stringstream json;
    c->toJSON(json);
    std::cout << json.str();
  } else {
    // FIXME: Will the frontend need some sort of error or is blank enough?
  }
}

int
main(int argc, char * argv[])
{
  GetColorMap alg = GetColorMap();

  // Turn off general logging since we're dumping text
  Log::pauseLogging();
  alg.executeFromArgs(argc, argv);
}
