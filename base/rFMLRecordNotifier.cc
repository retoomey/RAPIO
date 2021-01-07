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
    "Write fml file to directory for each new record.\n  Example: fml=/output  Example: fml= Write to {OutputDir}/code_index.fam";
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
FMLRecordNotifier::initialize(const std::string& params, const std::string& datalocation)
{
  // Default when given notify output location is empty
  if (params.empty()) {
    myOutputDir = URL(datalocation).getPath();
    myIndexPath = IOIndex::getIndexPath(myOutputDir);
    myOutputDir = myOutputDir + "/code_index.fam/";
  } else {
    myOutputDir = URL(params).getPath();
    myIndexPath = IOIndex::getIndexPath(myOutputDir);
  }

  myTempDir = myOutputDir + "/.working/"; // For file 'locking'
  myURL     = myOutputDir;
}

bool
FMLRecordNotifier::makeDirectories()
{
  bool success = true;

  // Make sure directory created
  if (!OS::mkdirp(myTempDir)) { // only one mkdirp since temp is subfolder
    LogSevere("Couldn't access/create fml directory: "
      << myTempDir << " so we can't notify!\n");
    success = false;
  }
  if (!Strings::beginsWith(myTempDir, myOutputDir)) { // Not a subfolder
    if (!OS::mkdirp(myOutputDir)) {
      LogSevere("Couldn't access/create fml directory: " << myOutputDir << " so we can't notify!\n");
      success = false;
    }
  }
  return success;
}

FMLRecordNotifier::~FMLRecordNotifier()
{ }


void
FMLRecordNotifier::writeRecord(const Record& rec, const std::string& file)
{
  if (!rec.isValid()) { return; }

  // Construct fml filename from the selections string
  const std::vector<std::string>& s = rec.getSelections();
  std::stringstream filename;
  filename << s[0];
  for (size_t i = 1; i < s.size(); ++i) {
    filename << '_' << s[i];
  }
  // First part of multi-write.  I'm gonna to append the factory
  // to the end of the fml filename.  This won't work for multiple
  // records from a single writer
  const std::vector<std::string>& p = rec.getBuilderParams();
  filename << '_' << p[0] << ".fml";

  // Write record to tmp .fml file ------------------------------
  const std::string tmpfilename = myTempDir + filename.str();
  std::ofstream ofp;
  ofp.open(tmpfilename.c_str());

  if (!ofp) {
    // Lazy try to make directories again...
    if (makeDirectories()) {
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
  ofp << "<meta>\n";
  rec.constructXMLMeta(ofp);
  ofp << "</meta>\n";

  ofp << "<item>\n";
  rec.constructXMLString(ofp, myIndexPath);
  ofp << "</item>\n";
  ofp.close();
  // End write record to tmp .fml file --------------------------

  // Rename from tmp to final
  const std::string outfilename = myOutputDir + filename.str();
  int result = rename(tmpfilename.c_str(), outfilename.c_str());
  if (result == 0) {
    LogDebug("FML Notify File -->>" << outfilename << "\n");
  } else {
    LogSevere("Unable to rename tmp .fml file " << tmpfilename << " to final location " << outfilename << "\n");
  }
} // FMLRecordNotifier::writeRecord
