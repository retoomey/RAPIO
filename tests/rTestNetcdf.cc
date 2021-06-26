#include "rTestNetcdf.h"
#include <iostream>
#include <sys/stat.h>

#include <dirent.h>
#include <sys/stat.h>

using namespace rapio;

/*
 * A RAPIO algorithm for testing speeds/sizes of different netcdf settings.
 *
 * Example, echoing from a code_index to multiple netcdf output:
 * ./rTestNetcdf -n="disable" -i /mydata/code_index.xml -o "netcdf=NETCDFTEST" --verbose=debug
 *
 * I might add more netcdf tests here later.  Build tests with 'make check'
 *
 * We copy from input to the output multiple times with each netcdf compression setting and type
 *
 * @author Robert Toomey
 **/
void
RAPIONetcdfTestAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO Netcdf Test Algorithm");
  o.setAuthors("Robert Toomey");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIONetcdfTestAlg::processOptions(RAPIOOptions& o)
{ }

void
RAPIONetcdfTestAlg::processNewData(rapio::RAPIOData& d)
{
  static bool firstFile = true;

  if (firstFile) {
    totalCount = 0;
    firstFile  = false;
  }

  // Look for any data the system knows how to read...
  auto r = d.datatype<rapio::DataType>();

  // Override output params for image output
  std::map<std::string, std::string> flags;

  // Prefix pattern for standard datatype/subtype/time
  const std::string prefix = DataType::DATATYPE_PREFIX;
  const std::string key    = r->getTypeName();

  if (r != nullptr) {
    // Standard echo of data to output, but for multiple
    LogInfo("-->Writing multiple netcdf files for: " << r->getTypeName() << "\n");

    // -------------------------------------------------------------------------
    // Write a netcdf3 file
    flags["ncflags"]    = "256"; // cmode for netcdf3 (see ucar netcdf docs)
    flags["fileprefix"] = prefix + "_n3";
    ProcessTimer z("");
    writeOutputProduct(key, r, flags);
    totalTimes[0] += z.getElapsedTime();

    // ------------------------------------------------------------------------
    // Write multiple netcdf4 files with different compression settings
    flags["ncflags"] = "4096"; // cmode for netcdf4 (see ucar netcdf docs)
    for (size_t i = 0; i < 10; i++) {
      ProcessTimer p("");
      const std::string s = std::to_string(i);
      flags["fileprefix"]    = prefix + "_n4_c" + s + "_";
      flags["deflate_level"] = s;
      //  std::this_thread::sleep_for(std::chrono::milliseconds(2400));
      writeOutputProduct(key, r, flags);
      totalTimes[i + 1] += p.getElapsedTime();
    }
    totalCount += 1;
  }

  std::cout << "------------------------------------------------------------\n";
  std::cout << "totalfiles           : " << totalCount << "\n";
  std::cout << "netcdf3              : " << totalTimes[0] << "\n";
  for (size_t i = 0; i < 10; i++) {
    std::cout << "netcdf4 compression " << i << ": " << totalTimes[i + 1] << "\n";
  }
} // RAPIONetcdfTestAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a netcdf test alg
  RAPIONetcdfTestAlg alg = RAPIONetcdfTestAlg();

  alg.executeFromArgs(argc, argv);
}
