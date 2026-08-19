#include "rMakeIndex.h"
#include "rOS.h"
#include "rStrings.h"
#include "rConfigIODataType.h"
#include "rIODataType.h"
#include "rFactory.h"
#include "rRecordQueue.h"
#include "rConfigRecord.h"
#include "rProcessTimer.h"

#include <fstream>
#include <regex>
#include <vector>
#include <functional>

using namespace rapio;

/** Concrete strategy for Regex-based extraction */
class RegexTimeParser : public TimeParser {
private:
  std::regex myPattern;
  std::string myFormat;
  std::function<std::string(const std::smatch&)> myExtractor;

public:
  RegexTimeParser(std::string name, std::regex p, std::string f, std::function<std::string(const std::smatch&)> ext)
    : TimeParser(name), myPattern(std::move(p)), myFormat(std::move(f)), myExtractor(std::move(ext)){ }

  bool
  scan(const std::string& str, Time& set) override
  {
    if (str.empty()) { return false; }
    std::smatch match;

    if (std::regex_search(str, match, myPattern)) {
      bool success = set.putString(myExtractor(match), myFormat);
      if (success) {
        myMatchCount++;
      }
      return success;
    }
    return false;
  }
};

/** Concrete strategy for the original fuzzy digit-counting fallback */
class FuzzyTimeParser : public TimeParser {
public:

  FuzzyTimeParser() : TimeParser("Fuzzy"){ }

  bool
  scan(const std::string& str, Time& set) override
  {
    if (str.empty()) { return false; }

    bool success = false;
    std::string v;

    // Filter to all digits
    std::copy_if(str.begin(), str.end(), std::back_inserter(v), [](char c) {
      return std::isdigit(c); // Include only digits
    });

    // We're expecting a couple of sizes.
    if (v.size() == 17) { // 19990503232153893
      success = set.putString(v, "%Y%m%d%H%M%S%/ms");
    } else if (v.size() == 14) { // 19990503232153
      success = set.putString(v, "%Y%m%d%H%M%S");
    }
    if (success) {
      myMatchCount++;
    }
    return success;
  }
};

/** Initialize the dir walker and load the parser cache into the member variable */
makeIndexDirWalker::makeIndexDirWalker(const std::string& what, MakeIndex * owner, const std::string& suffix)
  : myWhat(what), myOwner(owner), mySuffix(suffix)
{
  // Add Explicit Pattern: rap_130_YYYYMMDD_HHMM_000
  myParsers.push_back(std::make_shared<RegexTimeParser>("RAP",
    std::regex(R"(_(\d{8})_(\d{4})_)"),
    "%Y%m%d%H%M%S",
    [](const std::smatch& m) {
    return m[1].str() + m[2].str() + "00";
  }
  ));

  // Add future RegexTimeParsers here...

  // Add Fuzzy Fallback as the last resort
  myParsers.push_back(std::make_shared<FuzzyTimeParser>());
}

DirWalker::Action
makeIndexDirWalker::processRegularFile(const std::string& filePath, const struct stat * info)
{
  // FIXME: Add filters, right?  Might not want everything.

  // Step 1: Get the factory for this file name, so anything RAPIO can
  // read we can index with a builder.
  // FIXME: The code there and here feels a bit messy, API could clean up.
  //
  std::string factory = OS::getRootFileExtension(filePath);

  Strings::toLower(factory);
  std::string alias = ConfigIODataType::getIODataTypeFromSuffix(factory);
  std::shared_ptr<IODataType> f = Factory<IODataType>::get(alias, "IODataType:" + alias);

  // Step 2: Get the Time from the filename.
  if (f != nullptr) {
    std::string localFile = filePath.substr(getFileOffset());
    Time aTime;

    // Iterate over the member variable parsers
    bool parsedSuccessfully = false;
    for (const auto& parser : myParsers) {
      if (parser->scan(localFile, aTime)) {
        parsedSuccessfully = true;
        break;
      }
    }

    if (parsedSuccessfully) {
      fLogDebug("FILE:{} has BUILDER:({})", filePath, alias);
      // printPath("File: ", filePath, info);

      // Remove begining and end of the path
      // Maybe could optimize by doing higher
      // The display only handles 3 selections, do larger we'll stick
      // onto the first selection
      std::string subtree = filePath;
      Strings::removePrefix(subtree, getCurrentRoot());
      Strings::removeSuffix(subtree, localFile);
      std::vector<std::string> fields;
      Strings::splitWithoutEnds(subtree, '/', &fields);

      // The 'size' of subtree will match the depth now.
      // -------------------------------------------------------------------
      // OK, make a record.  Since we're not an algorithm the queue shouldn't
      // empty, but it will still sort. So we buffer the entire archive.
      std::vector<std::string> params;

      // First param is the builder
      params.push_back(alias);

      const bool fullMode = false; // do builder filename only
      if (fullMode) {
        params.push_back(filePath);
      } else {
        // Do the indexlocation and spaced out mode.
        params.push_back(Constants::IndexPathReplace);
        for (const auto& field : fields) {
          params.push_back(field); // MRMS/RAPIO just append together
        }
        params.push_back(localFile);
      }

      // Extract dataType and subType from the directory fields
      // Example: /KTLX/Reflectivity/00.50/ -> dataType="KTLX_Reflectivity", subType="00.50"
      std::string dataType = "unknown";
      std::string subType  = "default";

      if (!fields.empty()) {
        if (fields.size() == 1) {
          // Only one directory deep
          dataType = fields[0];
        } else {
          // Two or more directories deep
          subType = fields.back(); // Last directory is the subtype

          // Squash everything before the last directory into the dataType
          dataType = fields[0];
          for (size_t i = 1; i < fields.size() - 1; ++i) {
            dataType += "_" + fields[i];
          }
        }
      }

      // Create and enqueue the record
      Record rec(params, factory, aTime, dataType, subType);
      Record::theRecordQueue->addRecord(rec);
      // -------------------------------------------------------------------
    } else {
      // Couldn't time scan, so what next?
      fLogDebug("SKIP: Can't time parse {}, ok if not data.", localFile);
    }
  }

  return DirWalker::Action::CONTINUE;
} // makeIndexDirWalker::processRegularFile

DirWalker::Action
makeIndexDirWalker::processDirectory(const std::string& dirPath, const struct stat * info)
{
  // How to tell the level of directory we are at?  Should be somewhere to add to API,
  // or we check the path?
  fLogDebug("DIR: {} {}", dirPath, getDepth());
  // printPath("Dir: ", dirPath, info);
  return DirWalker::Action::CONTINUE; // Process the root
}

void
makeIndexDirWalker::printStats()
{
  // Iterate over the member variable parsers
  for (const auto& parser : myParsers) {
    // Could let the parser print
    fLogInfo("TimeParser '{}' matched {} files", parser->getName(), parser->getMatchCount());
  }
}

void
MakeIndex::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO/MRMS Make Index.  Any file RAPIO can currently read in will be indexed.");
  o.setAuthors("Valliappa Lakshman, Robert Toomey");

  o.optional("root", ".", "Root directory of scanning. Default is your current directory.");
  o.optional("outfile", "", "Output file for the generated index. Default output location is root directory.");
}

void
MakeIndex::processOptions(RAPIOOptions& o)
{
  myRoot = o.getString("root");
  if (myRoot == ".") {
    myRoot = OS::getCurrentDirectory();
  }
  myOutputFile = o.getString("outfile");
  if (myOutputFile == "") {
    // Make it the root directory location + code_index.xml
    myOutputFile = myRoot + "/code_index.xml";
  }
  fLogInfo("Root is {}, which will be indexlocation macro.", myRoot);
  fLogInfo("Output file is {}", myOutputFile);
}

void
MakeIndex::execute()
{
  ProcessTimer timer("Generate records");
  // Make a record queue for sorting/storing records.
  // FIXME: clean up design here of record queue.  We're hacking
  // into it here at moment.  Technically the API design issue is that
  // the queue and the event stuff are in one class.
  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(nullptr);

  Record::theRecordQueue = q;
  auto& myQueue = Record::theRecordQueue->getQueue();

  makeIndexDirWalker walk("INGEST", this, ".cache");

  walk.traverse(myRoot);
  fLogInfo("Processed {} total files.", walk.getFileCounter());
  fLogInfo("Gathered a total of {} records", myQueue.size());
  walk.printStats();

  // --------------------------------------------------------
  // Write xml output file
  //
  std::string indexPath = myRoot;
  std::ofstream ss(myOutputFile);

  if (!ss) {
    fLogSevere("Can't create/write '{}'", myOutputFile);
    return;
  }

  // ss << "<codeindex>\n";
  ss << "<codeindex";
  ss << " writer=\"rapio\"";
  ss << " date=\"" << Time::CurrentTime().getString() << "\"";
  ss << " size=\"" << myQueue.size() << "\"";
  ss << ">\n";
  while (!myQueue.empty()) {
    ss << "<item>\n";
    ConfigRecord::constructXMLString(myQueue.top(), ss, indexPath);
    myQueue.pop(); // Remove the processed record
    ss << "</item>\n";
  }
  ss << "</codeindex>\n";
  ss.close();

  fLogInfo("Wrote {}", myOutputFile);
  fLogInfo("{}", timer);
} // MakeIndex::execute

int
main(int argc, char * argv[])
{
  MakeIndex alg = MakeIndex();

  alg.executeFromArgs(argc, argv);
}
