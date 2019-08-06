#include "rIOURL.h"

#include "rFactory.h"
#include "rCompression.h"

#include <curl/curl.h>
#include <fstream>

using namespace rapio;
using namespace std;

bool IOURL::TRY_CURL  = true;
bool IOURL::GOOD_CURL = false;

namespace {
// Start curl stuff for remote access...
size_t
processData(void * ptr, size_t sz, size_t n, void * obj)
{
  std::vector<char> * data = (std::vector<char> *)obj;
  char * start = (char *) ptr;
  char * end   = start + sz * n;
  data->insert(data->end(), start, end);
  return (end - start);
}

// FIXME: Probably should be in own class for global use
class CurlConnection {
public:

  CurlConnection()
  {
    curl     = 0;
    _inited  = false;
    curlCode = curl_global_init(CURL_GLOBAL_ALL);

    bool success = (curlCode != 0);

    if (success) {
      LogSevere("Unable to initialize libcurl properly \n");
    } else {
      curl = curl_easy_init();
    }
  } // end of CurlConnection

  ~CurlConnection()
  {
    if (curl != 0) {
      curl_easy_cleanup(curl);
    }

    if (isValid()) {
      curl_global_cleanup();
    }
  }

  bool
  isValid()
  {
    return (curlCode == 0);
  }

  bool
  isInit()
  {
    return (_inited);
  }

  CURL *
  connection()
  {
    return (curl);
  }

  bool
  lazyInit()
  {
    bool ok = (curlCode == 0);

    if (!ok) {
      LogSevere("Unable to initialize libcurl properly \n");
      return (false);
    }

    // ok = ok && curl_easy_setopt(curl, CURLOPT_VERBOSE, 10) == 0;
    ok = ok &&
      curl_easy_setopt(connection(), CURLOPT_WRITEFUNCTION, processData) == 0;
    ok = ok && curl_easy_setopt(connection(), CURLOPT_NOSIGNAL, 1) == 0;
    ok = ok && curl_easy_setopt(connection(), CURLOPT_TIMEOUT, 30) == 0; // 30
                                                                         // seconds
    ok = ok && curl_easy_setopt(connection(), CURLOPT_CONNECTTIMEOUT, 30) == 0;

    if (!ok) {
      LogSevere("Unable to initialize libcurl library.\n");
      curl_easy_cleanup(connection());
      return (false);
    }

    _inited = true;
    return (true);
  } // end of lazyInit()

private:

  CURLcode curlCode;
  CURL * curl; // saves connections
  bool _inited;
}; // end of private class CurlConnection

static CurlConnection curlConnection;

// End curl stuff
}

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
        if (!(curlConnection.isValid() && curlConnection.isInit())) {
          LogInfo("Remote URL.  Initializing curl for remote request ability...\n");

          if (curlConnection.lazyInit()) {
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
      curlConnection.connection(), CURLOPT_URL, urls.c_str());

    if (ret != 0) {
      LogSevere("Opening " << urls << " failed with err=" << ret << "\n");
      return (-1);
    }
    std::vector<char> result; // FIXME: Do we need to copy?
    curl_easy_setopt(curlConnection.connection(), CURLOPT_WRITEDATA, &(result));
    ret = curl_easy_perform(curlConnection.connection());

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
