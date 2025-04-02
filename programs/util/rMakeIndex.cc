#include "rMakeIndex.h"

using namespace rapio;

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
  std::shared_ptr<IODataType> f = Factory<IODataType>::get(factory, "IODataType:" + factory);

  // Step 2: Get the Time from the filename.

  // -----------------------------------------------------------
  // Generalize into RAPIO, allow introduction of new parsers if
  // we're trying to read other formats with more complicated time
  // strings. For MRMS/HMRG I think the digit approach handles most
  // cases.
  //
  class TimeParser {
public:
    TimeParser(){ };

    /** Scan a fuzzy std::string and prepare for time conversion */
    virtual bool
    scan(const std::string& str, Time& set)
    {
      if (str.empty()) { return false; }

      bool success = false;

      // First, reduce to numbers and ignore everything else
      // For our standard format, this will either give us
      // 19990503-232153.893.netcdf.gz  --> 19990503232153893 (17 digits)
      // 19990503-232153.netcdf.gz --> 19990503232153 (13 digits)
      // REFL.20220202.222000.gz   --> 20220202222000 (13 digits)
      //
      // and this will crazy handle things like:
      // Somestuff_otherstuff_1990503___232153.893_KTLX_MORE.netcdf.gz
      // Assuming the numbers are in the 'standard' order we use.

      // Filter to all digits
      std::string v;

      std::copy_if(str.begin(), str.end(), std::back_inserter(v), [](char c) {
        return std::isdigit(c); // Include only digits
      });

      // We're expecting a couple of sizes.
      if (v.size() == 17) { // 19990503232153893
        success = set.putString(v, "%Y%m%d%H%M%S%/ms");
      } else if (v.size() == 14) { // 19990503232153
        success = set.putString(v, "%Y%m%d%H%M%S");
      }
      return success;
    }
  };

  if (f != nullptr) {
    std::string localFile = filePath.substr(getFileOffset());
    Time aTime;

    TimeParser test;
    if (test.scan(localFile, aTime)) {
      LogDebug("FILE:" << filePath << "has BUILDER:(" << factory << ")\n");
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
      //
      // OK, make a record.  Since we're not an algorithm the queue shouldn't
      // empty, but it will still sort. So we buffer the entire archive.
      // FIXME: This is very similar to rFileIndex.cc.  We should probably
      // make common code doing this, maybe in ConfigRecord
      std::vector<std::string> params;
      std::vector<std::string> selects;
      params.push_back(factory);

      const bool fullMode = false; // do builder filename only
      if (fullMode) {
        params.push_back(filePath);
      } else {
        // Do the indexlocation and spaced out mode.
        params.push_back(Constants::IndexPathReplace);
        for (size_t i = 0; i < fields.size(); ++i) {
          params.push_back(fields[i]); // MRMS/RAPIO just append together
        }
        params.push_back(localFile);
      }

      // The display handles three fields, so the second field we'll
      // compress the fields, giving us say:
      // KTLX, Reflectivity, 00.50 --> KTLX_Reflectivity 00.50
      size_t at = 0;
      selects.push_back(""); // time field
      if (fields.size() > 2) {
        at = fields.size() - 1; // skip later

        // Compress first n-2 items into one selection
        std::string out;
        for (size_t i = 0; i < at; ++i) {
          out += fields[i];
          if (i != at - 1) { out += "_"; }
        }
        selects.push_back(out);
      }
      for (size_t i = at; i < fields.size(); ++i) {
        selects.push_back(fields[i]);
      }

      Record rec(params, selects, aTime);
      Record::theRecordQueue->addRecord(rec);
      // -------------------------------------------------------------------
    } else {
      // Couldn't time scan, so what next?
      LogDebug("SKIP: Can't time parse " << localFile << ", ok if not data.\n");
    }
  }

  return DirWalker::Action::CONTINUE;
} // makeIndexDirWalker::processRegularFile

DirWalker::Action
makeIndexDirWalker::processDirectory(const std::string& dirPath, const struct stat * info)
{
  // How to tell the level of directory we are at?  Should be somewhere to add to API,
  // or we check the path?
  LogDebug("DIR:" << dirPath << " " << getDepth() << "\n");
  // printPath("Dir: ", dirPath, info);
  return DirWalker::Action::CONTINUE; // Process the root
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
  LogInfo("Root is " << myRoot << ", which will be {indexlocation} macro.\n");
  LogInfo("Output file is " << myOutputFile << "\n");
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
  LogInfo("Processed " << walk.getFileCounter() << " total files.\n");
  LogInfo("Gathered a total of " << myQueue.size() << " records.\n");

  // --------------------------------------------------------
  // Write xml output file
  //
  std::string indexPath = myRoot;
  std::ofstream ss(myOutputFile);

  if (!ss) {
    LogSevere("Can't create/write '" << myOutputFile << "'\n");
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
    myQueue.top().constructXMLString(ss, indexPath);
    myQueue.pop(); // Remove the processed record
    ss << "</item>\n";
  }
  ss << "</codeindex>\n";
  ss.close();

  LogInfo("Wrote " << myOutputFile << "\n");
  LogInfo(timer << "\n");
} // MakeIndex::execute

int
main(int argc, char * argv[])
{
  MakeIndex alg = MakeIndex();

  alg.executeFromArgs(argc, argv);
}
