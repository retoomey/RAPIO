#include "rFMLRecordNotifier.h"

#include "rOS.h"
#include "rIOIndex.h"
#include "rError.h"
#include "rFactory.h"
#include "fstream"
#include "rStrings.h"
#include "rConfigRecord.h"

#include <stdio.h>

using namespace rapio;

namespace rapio {
/** Create a new instance of FMLRecordNotifier.
 * @ingroup rapio_io
 * @brief Create a new instance of FMLRecordNotifier
 * */
class FMLRecordNotifierCreator : public RecordNotifierCreator {
public:
  std::shared_ptr<RecordNotifierType>
  create(const std::string& params) override
  {
    auto ptr = std::make_shared<FMLRecordNotifier>();

    ptr->initialize(params); // Could pass to constructor now
    return ptr;
  }

  /** Help for created objects */
  virtual std::string
  getHelpString(const std::string& fkey) override
  {
    return
      "Write fml file to directory for each new record.\n  Example: fml=/output to have ALL writers write fml records to that directory.\n  Example: fml= Write to {OutputDir}/code_index.fam for each writer's path.  So image=myimages would write to myimages/code_index.fam and netcdf=mynetcdf would write to mynetcdf/code_index.fam";
  }
};
}

void
FMLRecordNotifier::introduceSelf()
{
  // Tiny class dedicated to creating new instances, since we can have more than one
  std::shared_ptr<FMLRecordNotifierCreator> newOne = std::make_shared<FMLRecordNotifierCreator>();
  Factory<RecordNotifierCreator>::introduce("fml", newOne);
}

void
FMLRecordNotifier::initialize(const std::string& params)
{
  if (!params.empty()) {
    // Override with params as assumed directory.
    // FIXME: override path will affect ALL writers.  We could extend the parameter
    // to allow putting different writers in different places.
    myOutputDir = URL(params).getPath();
  } else {
    myOutputDir = ""; // Just to make it clear
  }
}

bool
FMLRecordNotifier::makeDirectories(const std::string& outdir, const std::string& tempdir)
{
  bool success = true;

  // Make sure directory created
  if (!OS::mkdirp(tempdir)) { // only one mkdirp since temp is subfolder
    fLogSevere("Couldn't access/create fml directory: {} so we can't notify!", tempdir);
    success = false;
  }
  if (!Strings::beginsWith(tempdir, outdir)) { // Not a subfolder
    if (!OS::mkdirp(outdir)) {
      fLogSevere("Couldn't access/create fml directory: {} so we can't notify!", outdir);
      success = false;
    }
  }
  return success;
}

FMLRecordNotifier::~FMLRecordNotifier()
{ }

void
FMLRecordNotifier::getOutputFolder(std::map<std::string, std::string>& outputParams,
  std::string& outputDir, std::string& indexLocation)
{
  // Maybe we can cache this somehow for speed improvements
  // Typically the output directory will stay the same. Someone could
  // change the outputParams in theory

  const std::string outputinfo = outputParams["outputfolder"];

  // We assume the outputinfo is a directory.  Override with myOutputDir if requested
  // on the notifier param line:
  outputDir = myOutputDir.empty() ? outputinfo : myOutputDir;

  // Now add any require subfolder to our output dir.
  const std::string subfolder = outputParams["outputsubfolder"];

  if (!subfolder.empty()) {
    outputDir = outputDir + "/" + subfolder;
  }

  outputDir += "/";

  // Find the macro indexLocation from cache or create it
  // We have to check this each time because the output directory
  // can be different for each writer.  We can cache per output path though
  auto indexPath = myIndexPaths.find(outputDir);

  if (indexPath == myIndexPaths.end()) {
    indexLocation = myIndexPaths[outputDir] = IOIndex::getIndexPath(outputDir);
  } else {
    indexLocation = indexPath->second;
  }

  // Append code_index.fam always for fam (We decided this is less confusing in operations)
  outputDir = outputDir + "code_index.fam/"; // ...then put in subfolder of default output
} // FMLRecordNotifier::getOutputFolder

void
FMLRecordNotifier::writeMessage(std::map<std::string, std::string>& outputParams, const Message& m)
{
  // Promote the message to a record, send it on it's way
  Record r(m);

  writeRecord(outputParams, r);
}

void
FMLRecordNotifier::writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec)
{
  std::string outputDir, indexLocation;

  // Calculate the output folder location .../code_index.fml
  getOutputFolder(outputParams, outputDir, indexLocation);

  // ---------------------------------------------------
  // Get the fml filename.
  // std::stringstream filename;
  // filename << rec.getIDString() << ".fml";
  std::string filename = rec.getIDString() + ".fml";

  // Write record to tmp .fml file ------------------------------
  std::string tempDir = outputDir + ".working/"; // For file 'locking'
  // const std::string tmpfilename = tempDir + filename.str();
  const std::string tmpfilename = tempDir + filename;
  std::ofstream ofp;

  ofp.open(tmpfilename.c_str());

  if (!ofp) {
    // Lazy try to make directories again...
    if (makeDirectories(outputDir, tempDir)) {
      ofp.open(tmpfilename.c_str());
    }

    if (!ofp) {
      fLogSevere("Unable to create {}", tmpfilename);
      return;
    }
  }

  // We're responsible for the meta and the item part, this
  // allows single meta for example for a large file containing
  // multiple items.  FAM just writes one
  // WDSSII breaking bad on this...plus guess XML doesn't want multiple head nodes?  Interesting
  //  ofp << "<meta>\n";
  //  ConfigRecord::constructXMLMeta(rec, ofp);
  //  ofp << "</meta>\n";

  ofp << "<item>\n";
  ConfigRecord::constructXMLString(rec, ofp, indexLocation);
  ofp << "</item>\n";
  ofp.close();
  // End write record to tmp .fml file --------------------------

  // Rename from tmp to final
  const std::string outfilename = outputDir + filename;
  int result = rename(tmpfilename.c_str(), outfilename.c_str());

  if (result == 0) {
    // -----------------------------------------------------------------------
    // Post write command on a written file (comes from postfml key)
    const std::string postCommand = outputParams["postfml"];
    OS::runCommandOnFile(postCommand, outfilename, true);
  } else {
    fLogSevere("Unable to rename tmp .fml file {} to final location {}", tmpfilename, outfilename);
  }
} // FMLRecordNotifier::writeRecord
