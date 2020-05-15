#include "rFMLRecordNotifier.h"

#include "rOS.h"
#include "rIOIndex.h"
#include "rError.h"
#include "rFactory.h"
#include "fstream"
#include "rStrings.h"

#include <stdio.h>

using namespace rapio;

void
FMLRecordNotifier::setURL(URL at, URL datalocation)
{
  // Default when given notify output location is empty
  if (at.empty()) {
    myOutputDir = datalocation.getPath();
  } else {
    myOutputDir = at.getPath();
  }

  myIndexPath = IOIndex::getIndexPath(myOutputDir);
  myOutputDir = myOutputDir + "/code_index.fam/";
  myTempDir   = myOutputDir + ".working/"; // For file 'locking'
  myURL       = myOutputDir;

  //  makeDirectories();
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
FMLRecordNotifier::introduceSelf()
{
  std::shared_ptr<FMLRecordNotifier> newOne = std::make_shared<FMLRecordNotifier>();
  Factory<RecordNotifier>::introduce("fml", newOne);
}

void
FMLRecordNotifier::writeRecord(const Record& rec)
{
  if (!rec.isValid()) { return; }

  // Construct fml filename from the selections string
  const std::vector<std::string>& s = rec.getSelections();
  std::stringstream filename;
  filename << s[0];
  for (size_t i = 1; i < s.size(); ++i) {
    filename << '_' << s[i];
  }
  filename << ".fml";

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
