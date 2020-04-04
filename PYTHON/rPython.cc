// Robert Toomey
// March 2020
// A Wrapper algorithm for calling simple python processing
// scripts

#include <RAPIO.h>

using namespace rapio;

/** Create your algorithm as a subclass of RAPIOAlgorithm */
class RPythonAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create a wrapper */
  RPythonAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o)
  {
    o.setHeader(Constants::RAPIOHeader + "Utility Algorithm");
    o.setDescription("Wrapper for a Python script of your choosing for processing grid data. ");
    o.setAuthors("Robert Toomey");

    o.optional("P", "/usr/bin/python", "Location of your Python executable");
    o.require("S", "myscript.py", "The path to your Python script");
    o.optional("A", "", "Extra arguments to pass to your Python script");
  };

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o)
  {
    myPythonPath   = o.getString("P");
    myPythonScript = o.getString("S");
    myPythonArgs   = o.getString("A");
  };

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d)
  {
    // FIXME: General method I think.  This is useful to print out
    std::string s = "Data received: ";
    auto sel      = d.record().getSelections();
    for (auto& s1:sel) {
      s = s + s1 + " ";
    }
    LogInfo(s << " from index " << d.matchedIndexNumber() << "\n");

    // FIXME: Only handling DataGrids at moment
    auto r = d.datatype<rapio::DataGrid>();
    if (r != nullptr) {
      LogInfo("Incoming data grid, sending to python...\n");
      auto output = OS::runDataProcess(myPythonPath + " " + myPythonScript + " " + myPythonArgs, r);
      LogInfo("PYTHON: " << myPythonScript << "\n");
      for (auto v:output) {
        // LogInfo("PYTHON: " << v << "\n");
        std::cout << "--" << v << "\n";
      }
      // FIXME: Maybe we don't want to write anything?  I Think -O could have a complete turn off option
      // Note if python modified the grid, it will write out here
      // If your python just creates tables or something..gonna need some flag to know so we
      // just make notifications?  So much potential
      LogInfo("--->Writing " << r->getTypeName() << " product to output\n");
      writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
    }
  };

protected:

  /** Path to python distribution to use */
  std::string myPythonPath;

  /** Path to the python file to execute */
  std::string myPythonScript;

  /** Path to the python file to execute */
  std::string myPythonArgs;
};

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  RPythonAlg alg = RPythonAlg();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
