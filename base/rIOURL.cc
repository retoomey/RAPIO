#include "rIOURL.h"

#include "rFactory.h"
#include "rCompression.h"

#include <curl/curl.h>
#include <fstream>

using namespace rapio;
using namespace std;

bool IOURL::TRY_CURL  = true;
bool IOURL::GOOD_CURL = false;
std::shared_ptr<CurlConnection> IOURL::myCurlConnection;

int
IOURL::read(const URL& url, Buffer& buf)
{
  if (readRaw(url, buf) == -1) {
    return (-1);
  }

  // Compressed data
  //  ------------------------------------------------------------
  // After pulling data, uncompress if wanted
  const std::string suffix(url.getSuffixLC());

  if (suffix == "gz") {
    buf = Compression::uncompressGZ(buf);
  } else if (suffix == "bz2") {
    buf = Compression::uncompressBZ2(buf);
  } else if (suffix == "z") {
    LogSevere("Z data unsupported\n");
    return (-1);

    /*  Probably can make this work but I don't like copying too much
     * Buffer buf;
     *
     * source->read(buf);
     * const std::string filename(getUniqueTemporaryFile("zfile"));
     * const std::string zfilename(filename + ".Z");
     * FILE *fp = fopen(zfilename.c_str(), "w+");
     * fwrite(&buf.data().front(), 1, buf.size(), fp);
     * fclose(fp);
     * std::string command("gunzip " + zfilename);
     * ::system(command.c_str());
     * IOURL::read(filename, myBuf);
     * unlink(filename.c_str());
     */
  }
  return (buf.getLength());
}

int
IOURL::readRaw(const URL& url, Buffer& buf)
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
    std::vector<char> result; // FIXME: Do we need to copy?
    curl_easy_setopt(myCurlConnection->connection(), CURLOPT_WRITEDATA, &(result));
    ret = curl_easy_perform(myCurlConnection->connection());

    if (ret != 0) {
      LogSevere("Reading " << url << " failed with err=" << ret << "\n");
      return (-1);
    }
    buf = Buffer(result);
  } else {
    // Local file ingest
    // ------------------------------------------------------------
    std::vector<char>& c = buf.data();
    std::ifstream file(url.path);

    if (file) {
      // Get size of file
      file.seekg(0, std::ios::end);
      std::streampos length = file.tellg();
      file.seekg(0, std::ios::beg);

      c.resize(length);
      file.read(&c[0], length);
    } else {
      LogSevere("FAILED local file read " << url << "\n");
    }
  }
  return (buf.getLength());
} // IOURL::readRaw

int
IOURL::read(const URL& url, std::string& s)
{
  s.clear();

  Buffer b;
  read(url, b);
  s.insert(s.end(), b.data().begin(), b.data().end());

  return (s.size());
}
