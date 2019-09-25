#include "rIOURL.h"

#include "rFactory.h"
#include "rCompression.h"
#include "rError.h"

#include <curl/curl.h>
#include <fstream>

using namespace rapio;
using namespace std;

bool IOURL::TRY_CURL  = true;
bool IOURL::GOOD_CURL = false;
std::shared_ptr<CurlConnection> IOURL::myCurlConnection;

int
IOURL::read(const URL& url, std::vector<char>& buf)
{
  // FIXME: Maybe a suffix 'loop' for processing nested formats

  // Pull raw data into rawData buffer
  std::vector<char> rawData;
  if (readRaw(url, rawData) == -1) {
    return (-1);
  }

  //  ------------------------------------------------------------
  // Choose a decompressor
  const std::string suffix(url.getSuffixLC());
  bool (* decompress)(std::vector<char>& in, std::vector<char>& out) = nullptr;
  if (suffix == "gz") {
    decompress = Compression::uncompressGzip;
  } else if (suffix == "bz2") {
    decompress = Compression::uncompressBzip2;
  } else if (suffix == "lzma") {
    decompress = Compression::uncompressLzma;
  } else if (suffix == "z") {
    decompress = Compression::uncompressZlib;
  }

  // No compression known, move rawData to output.
  if (decompress == nullptr) {
    buf = std::move(rawData);
  } else {
    // Try to decompress with the choosen decompressor
    std::vector<char> output;
    if (!(*decompress)(rawData, output)) {
      return -1;
    }
    buf = std::move(output);
  }
  return (buf.size());
} // IOURL::read

int
IOURL::readRaw(const URL& url, std::vector<char>& buf)
{
  // Web ingest ------------------------------------------------------------
  if (url.isLocal() == false) {
    if (!GOOD_CURL) { // 99.999% skip
      // Try to set up curl if we haven't tried
      if (TRY_CURL) {
        if (myCurlConnection == nullptr) {
          myCurlConnection = std::make_shared<CurlConnection>();
        }
        if (!(myCurlConnection->isValid() && myCurlConnection->isInit())) {
          LogInfo("Remote URL.  Initializing curl for remote request ability...\n");

          if (myCurlConnection->lazyInit()) {
            LogInfo("...curl initialized\n");
            GOOD_CURL = true;
          }
        } else {
          return (-1); // Curl failed :(
        }
        TRY_CURL = false; // Don't try again
      } else {
        LogSevere("Can't read remote URL without curl\n");
        return (-1);
      }
    }

    // Read file with curl
    std::string urls = url.toString();
    CURLcode ret     = curl_easy_setopt(
      myCurlConnection->connection(), CURLOPT_URL, urls.c_str());

    if (ret != 0) {
      LogSevere("Opening " << urls << " failed with err=" << ret << "\n");
      return (-1);
    }
    curl_easy_setopt(myCurlConnection->connection(), CURLOPT_WRITEDATA, &(buf));
    ret = curl_easy_perform(myCurlConnection->connection());

    if (ret != 0) {
      LogSevere("Reading " << url << " failed with err=" << ret << "\n");
      return (-1);
    }
  } else {
    // Local file ingest
    // ------------------------------------------------------------
    // std::vector<char>& c = buf.data();
    std::ifstream file(url.path);

    if (file) {
      // Get size of file
      file.seekg(0, std::ios::end);
      std::streampos length = file.tellg();
      file.seekg(0, std::ios::beg);

      // c.resize(length);
      buf.resize(length);
      file.read(&buf[0], length);
    } else {
      LogSevere("FAILED local file read " << url << "\n");
    }
  }
  return (buf.size());
} // IOURL::readRaw
