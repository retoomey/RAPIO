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
  o.optional("maxcount", "0", "Maximum files to process before stopping, or 0 forever.");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIONetcdfTestAlg::processOptions(RAPIOOptions& o)
{
  myMaxCount = o.getInteger("maxcount");
}

void
RAPIONetcdfTestAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  auto r = d.datatype<rapio::DataType>();

  if (r != nullptr) {
    // Override output params for image output
    std::map<std::string, std::string> flags;

    // Prefix pattern for standard datatype/subtype/time
    const std::string prefix = DataType::DATATYPE_PREFIX;
    const std::string key    = r->getTypeName();

    // Standard echo of data to output, but for multiple
    LogInfo("-->Writing multiple netcdf files for: " << r->getTypeName() << "\n");

    // -------------------------------------------------------------------------
    // Write a netcdf3 file
    flags["ncflags"]    = "256"; // cmode for netcdf3 (see ucar netcdf docs)
    flags["fileprefix"] = prefix + "_n3";
    ProcessTimer z("");
    writeOutputProduct(key, r, flags);
    totalSums[0].add(z);

    // Write a xz file, curious on speed vs native netcdf compression
    flags["compression"] = "xz";
    ProcessTimer zp("");
    writeOutputProduct(key, r, flags);
    totalSums[1].add(zp);
    flags["compression"] = "";

    // ------------------------------------------------------------------------
    // Write multiple netcdf4 files with different compression settings
    flags["ncflags"] = "4096"; // cmode for netcdf4 (see ucar netcdf docs)
    for (size_t i = 0; i < 10; i++) {
      ProcessTimer p("writing");
      const std::string s = std::to_string(i);
      flags["fileprefix"]    = prefix + "_n4_c" + s + "_";
      flags["deflate_level"] = s;
      //  std::this_thread::sleep_for(std::chrono::milliseconds(2400));
      writeOutputProduct(key, r, flags);
      totalSums[i + 2].add(p);
      // LogInfo(p << "\n");
    }
  }

  std::cout << "------------------------------------------------------------\n";
  std::cout << "totalfiles           : " << totalSums[0].getCount() << "\n";
  std::cout << "netcdf3              : " << totalSums[0] << "\n";
  std::cout << "netcdf3+lz           : " << totalSums[1] << "\n";
  for (size_t i = 0; i < 10; i++) {
    std::cout << "netcdf4 compression " << i << ": " << totalSums[i + 2] << "\n";
  }

  // Dump current memory usage
  double vm_usage, resident_set;

  OS::getProcessSize(vm_usage, resident_set);

  std::cout << "Virtual Memory: " << vm_usage << " KB Resident: " << resident_set << " KB\n";

  if (myMaxCount > 0) {
    if (totalSums[0].getCount() >= myMaxCount) {
      LogInfo("Ending test due to maxcount of " << myMaxCount << "\n");
      exit(0);
    }
  }
} // RAPIONetcdfTestAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a netcdf test alg
  RAPIONetcdfTestAlg alg = RAPIONetcdfTestAlg();

  alg.executeFromArgs(argc, argv);
}
