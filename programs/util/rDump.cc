#include "rDump.h"

using namespace rapio;

void
Dump::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Dump datatype to text tool");
  o.setHeader(""); // turn off for first pass
  o.setExample("test.netcdf // ncdump style");

  // Text only treat like a single file...
  o.setTextOnlyMacro("-i file=%s -o=/tmp");
}

void
Dump::processOptions(RAPIOOptions& o)
{
  // Hack:
  // Turn logging back on if no macro (direct text on line)
  // FIXME: We kinda need to buffer the 'special' startup logging, so we
  // can show issues.
  if (!isMacroApplied()) {
    Log::restartLogging();
    fLogInfo("Reenabling logging due to non direct text command line options.");
  }
}

void
Dump::processNewData(rapio::RAPIOData& d)
{
  fLogSevere("Process called..");
  // Look for any data the system knows how to read
  auto data = d.datatype<rapio::DataType>();

  if (data == nullptr) {
    fLogSevere("Failed to get valid DataType");
    return;
  }

  // Default output
  data->setReadFactory("text"); // setReadFactory API a bit bleh

  // Slightly Sneaky.  If the macro was expanded, then we dump to screen
  // rdump somefile.raw ---> expanded to rdump -i=FILE=somefile.raw -o=/tmp
  std::map<std::string, std::string> myOverride;

  if (isMacroApplied()) {
    myOverride["console"] = "true";
    writeOutputProduct(data->getTypeName(), data, myOverride); // Typename will be replaced by -O filters
    // force exit in case they put -r or something which makes no sense really here
    exit(0);
  } else {
    // Not from macro, so treat normal for standard real time ability..
    // rdump -i=code_index -o=/tmp  the usual of any algorithm
    myOverride["console"] = "";
    writeOutputProduct(data->getTypeName(), data, myOverride); // Typename will be replaced by -O filters
  }
}

int
main(int argc, char * argv[])
{
  Dump alg = Dump();

  // FIXME: Chicken egg logging issue.  If dumping text we probably only want
  // that text...if real time we'll want standard logging.  Hack for now
  // by turning off logging.  This doesn't affect the 'special' logging of options
  Log::pauseLogging();
  alg.executeFromArgs(argc, argv);
}
