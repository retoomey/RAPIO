#include "rFMLIndex.h"

#include "rIOIndex.h"
#include "rError.h"
#include "rRecordQueue.h"
#include "rIODataType.h"
#include "rDirWatcher.h"
#include "rOS.h"
#include "rConfigRecord.h"

using namespace rapio;

/** Default constant for a FAM polling index */
const std::string FMLIndex::FMLINDEX_FAM = "ifam";

/** Default constant for a polling FML index */
const std::string FMLIndex::FMLINDEX_POLL = "ipoll";

FMLIndex::~FMLIndex()
{ }

FMLIndex::FMLIndex(
  const std::string  & protocol,
  const URL          & aURL,
  const TimeDuration & maximumHistory) :
  FileIndex(protocol, aURL, maximumHistory)
{ }

std::string
FMLIndex::getHelpString(const std::string& fkey)
{
  if (fkey == FMLINDEX_FAM) {
    return "Use inotify to watch a directory for .fml metadata files.\n  Example: " + FMLINDEX_FAM + "=code_index.fam";
  } else {
    return "Use polling to watch a directory for .fml metadata files.\n  Example: " + FMLINDEX_POLL + "=code_index.fam";
  }
}

bool
FMLIndex::canHandle(const URL& url, std::string& protocol, std::string& indexparams)
{
  // We'll claim any missing protocol with a .fam ending for legacy support
  if (protocol.empty()) {
    std::string suffix = OS::getRootFileExtension(url.toString());
    //    std::string suffix = url.getSuffixLC();
    if (suffix == "fam") { // Not a magic string, it's the end of code_index.fam, etc.
      protocol = FMLIndex::FMLINDEX_FAM;
      return true;
    }
  }
  return false;
}

bool
FMLIndex::wantFile(const std::string& path)
{
  // Ignore the 'system' stuff '.' and '..'
  if ((path.size() < 2) || (path[0] == '.')) {
    return false;
  }
  bool wanted = false;

  // ".fml" only please.
  // This is very picky.  Could make it case insensitive
  const size_t s = path.size();

  if ((s > 3) && (path[s - 4] == '.') &&
    (path[s - 3] == 'f') &&
    (path[s - 2] == 'm') &&
    (path[s - 1] == 'l')
  )
  {
    wanted = true;
  }
  return (wanted);
}

bool
FMLIndex::initialRead(bool realtime, bool archive)
{
  // Change protocol to the file ones and let file
  // index do the work
  if (myProtocol == FMLIndex::FMLINDEX_FAM) {
    myProtocol = FileIndex::FileINDEX_FAM;
  } else if (myProtocol == FMLINDEX_POLL) {
    myProtocol = FileIndex::FileINDEX_POLL;
  }
  return FileIndex::initialRead(realtime, archive);
} // FMLIndex::initialRead

bool
FMLIndex::fileToRecord(const std::string& filename, Record& rec)
{
  // For now, tell .fml to be parsed as xml builder
  auto doc = IODataType::read<PTreeData>(filename, "xml");

  if (doc == nullptr) {
    // Error not needed, XML will complain
    // LogSevere("Unable to parse " << filename << "\n");
    return false;
  }
  auto tree = doc->getTree();

  /*
   * auto meta = doc->getChildOptional("meta");
   * FIXME: Use meta data?
   */
  try{
    auto item = tree->getChild("item");
    return (ConfigRecord::readXML(rec, item, myIndexPath, getIndexLabel()));
  }catch (const std::exception& e) {
    LogSevere("Missing item tag in FML record\n");
  }
  return false;
}

void
FMLIndex::handleFile(const std::string& filename)
{
  if (wantFile(filename)) {
    Record rec;
    if (fileToRecord(filename, rec)) {
      // Add to the record queue.  Never process here directly.  The queue
      // will call our addRecord when it's time to do the work
      Record::theRecordQueue->addRecord(rec);
    }
  }
}

void
FMLIndex::introduceSelf()
{
  // input FAM ingestor
  std::shared_ptr<IndexType> newOne = std::make_shared<FMLIndex>();

  // Handle a fml index getting stuff from fam
  IOIndex::introduce(FMLINDEX_FAM, newOne);
  // Handle a fml index getting stuff from a directory poll
  IOIndex::introduce(FMLINDEX_POLL, newOne);
}

std::shared_ptr<IndexType>
FMLIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<FMLIndex> result = std::make_shared<FMLIndex>(
    protocol,
    URL(indexparams),
    maximumHistory);

  return (result); // Factory handles isValid now...
}
