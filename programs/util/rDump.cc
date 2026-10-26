#include "rDump.h"

using namespace rapio;

void
Dump::declareOptions(RAPIOOptions& o)
{
  // Logging tweaks.  Here it's early enough.
  Log::setUseStdErr(true);
  // We want errors ONLY to not mess up the output.  But we want to know when
  // something fails horribly like a missing module, etc.
  o.setDefaultValue("verbose", "severe"); 

  o.setDescription("Dump datatype to text tool");
  o.setHeader(""); // turn off for first pass
  o.setExample("test.netcdf // ncdump style");

  // Text only treat like a single file...
  o.setTextOnlyMacro("-i file=%s -o=/tmp");
}

void
Dump::processOptions(RAPIOOptions&)
{
}

void
Dump::processNewData(rapio::RAPIOData& d)
{
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
  IOConfig myOverride;

  // If the macro was expanded, we dump to screen.
  // Otherwise, treat as normal for standard real-time ability.
  if (isMacroApplied()) {
    myOverride.set("console", "true");
  } else {
    myOverride.set("console", "");
  }

  // Typename will be replaced by -O filters if applicable
  writeOutputProduct(data->getTypeName(), data, myOverride);
}

int
main(int argc, char * argv[])
{
  Dump alg = Dump();
  alg.executeFromArgs(argc, argv);
}
