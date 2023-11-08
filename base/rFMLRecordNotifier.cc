#include "rFMLRecordNotifier.h"

#include "rOS.h"
#include "rIOIndex.h"
#include "rError.h"
#include "rStaticMethodFactory.h"
#include "fstream"
#include "rStrings.h"

#include <stdio.h>

using namespace rapio;

/** Static factory method */
std::shared_ptr<RecordNotifierType>
FMLRecordNotifier::create()
{
  std::shared_ptr<FMLRecordNotifier> result = std::make_shared<FMLRecordNotifier>();

  return (result);
}

std::string
FMLRecordNotifier::getHelpString(const std::string& fkey)
{
  return
    "Write fml file to directory for each new record.\n  Example: fml=/output to have ALL writers write fml records to that directory.\n  Example: fml= Write to {OutputDir}/code_index.fam for each writer's path.  So image=myimages would write to myimages/code_index.fam and netcdf=mynetcdf would write to mynetcdf/code_index.fam";
}

void
FMLRecordNotifier::introduceSelf()
{
  // No need to store a special object here, could if wanted
  //  std::shared_ptr<FMLRecordNotifier> newOne = std::make_shared<FMLRecordNotifier>();
  //  Factory<RecordNotifierType>::introduce("fml", newOne);
  StaticMethodFactory<RecordNotifierType>::introduce("fml", &FMLRecordNotifier::create);
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
    LogSevere("Couldn't access/create fml directory: "
      << tempdir << " so we can't notify!\n");
    success = false;
  }
  if (!Strings::beginsWith(tempdir, outdir)) { // Not a subfolder
    if (!OS::mkdirp(outdir)) {
      LogSevere("Couldn't access/create fml directory: " << outdir << " so we can't notify!\n");
      success = false;
    }
  }
  return success;
}

FMLRecordNotifier::~FMLRecordNotifier()
{ }


void
FMLRecordNotifier::writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec)
{
  if (!rec.isValid()) { return; }

  const std::string outputinfo = outputParams["outputfolder"];

  // We assume the outputinfo is a directory.  Override with myOutputDir if requested
  // on the notifier param line:
  std::string outputDir = myOutputDir.empty() ? outputinfo : myOutputDir;

  outputDir += "/";

  // Find the macro indexLocation from cache or create it
  // We have to check this each time because the output directory
  // can be different for each writer.  We can cache per output path though
  std::string indexLocation;
  auto indexPath = myIndexPaths.find(outputDir);

  if (indexPath == myIndexPaths.end()) {
    indexLocation = myIndexPaths[outputDir] = IOIndex::getIndexPath(outputDir);
  } else {
    indexLocation = indexPath->second;
  }

  // Append code_index.fam if it's default directory
  // FIXME: I think it's correct to add after the indexpath lookup...should confirm this
  if (myOutputDir.empty()) {                   // Not a forced location to write to...
    outputDir = outputDir + "code_index.fam/"; // ...then put in subfolder of default output
  }

  // Construct fml filename from the selections string
  const std::vector<std::string>& s = rec.getSelections();
  std::stringstream filename;

  filename << s[0];
  for (size_t i = 1; i < s.size(); ++i) {
    filename << '_' << s[i];
  }

  // Add sourcename to avoid data stomping with multi-radar output
  const std::string sourceName = rec.getSourceName();

  if (!sourceName.empty()) {
    filename << '_' << sourceName;
  }

  // First part of multi-write.  I'm gonna to append the factory
  // to the end of the fml filename.  This won't work for multiple
  // records from a single writer
  const std::vector<std::string>& p = rec.getBuilderParams();

  filename << '_' << p[0] << ".fml";

  // Write record to tmp .fml file ------------------------------
  std::string tempDir = outputDir + ".working/"; // For file 'locking'
  const std::string tmpfilename = tempDir + filename.str();
  std::ofstream ofp;

  ofp.open(tmpfilename.c_str());

  if (!ofp) {
    // Lazy try to make directories again...
    if (makeDirectories(outputDir, tempDir)) {
      ofp.open(tmpfilename.c_str());
    }

    if (!ofp) {
      LogSevere("Unable to create " << tmpfilename << "\n");
      return;
    }
  }

  // We're responsible for the meta and the item part, this
  // allows single meta for example for a large file containing
  // multiple items.  FAM just writes one
  // WDSSII breaking bad on this...plus guess XML doesn't want multiple head nodes?  Interesting
  //  ofp << "<meta>\n";
  //  rec.constructXMLMeta(ofp);
  //  ofp << "</meta>\n";

  ofp << "<item>\n";
  rec.constructXMLString(ofp, indexLocation);
  ofp << "</item>\n";
  ofp.close();
  // End write record to tmp .fml file --------------------------

  // Rename from tmp to final
  const std::string outfilename = outputDir + filename.str();
  int result = rename(tmpfilename.c_str(), outfilename.c_str());

  if (result == 0) {
    LogDebug("FML Notify File -->>" << outfilename << "\n");
    // -----------------------------------------------------------------------
    // Post write command on a written file (comes from postfml key)
    const std::string postCommand = outputParams["postfml"];
    OS::runCommandOnFile(postCommand, outfilename, true);
  } else {
    LogSevere("Unable to rename tmp .fml file " << tmpfilename << " to final location " << outfilename << "\n");
  }
} // FMLRecordNotifier::writeRecord
