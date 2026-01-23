#include "rGetLegend.h"

using namespace rapio;

void
GetLegend::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Generate SVG legend for a given filename");
  o.setHeader(""); // turn off for first pass
  o.setExample("test.netcdf // ncdump style");

  // Width of the legend
  o.optional("width", "80", "Width of the legend (pixels)");
  o.addGroup("width", "SVG settings");

  o.optional("height", "600", "Height of the legend (pixels)");
  o.addGroup("height", "SVG settings");

  // Text only treat like a single file...
  o.setTextOnlyMacro("-i file=%s -o=/tmp");
}

void
GetLegend::processOptions(RAPIOOptions& o)
{
  // FIXME: We'll add more controls/params over time.  We need
  // to also sync with webserver probably.
  myWidth  = o.getInteger("width");
  myHeight = o.getInteger("height");
}

void
GetLegend::processNewData(rapio::RAPIOData& d)
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
    // FIXME: 'could' in in datatype maybe
    auto c     = data->getColorMap();
    auto units = data->getUnits();

    std::stringstream svg;
    c->toSVG(svg, units, myWidth, myHeight);
    std::cout << svg.str();
  } else {
    // FIXME: Will the frontend need some sort of error or is blank enough?
  }
}

int
main(int argc, char * argv[])
{
  GetLegend alg = GetLegend();

  // Turn off general logging since we're dumping text
  Log::pauseLogging();
  alg.executeFromArgs(argc, argv);
}
