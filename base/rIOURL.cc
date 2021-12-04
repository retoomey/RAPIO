#include "rIOURL.h"

#include "rFactory.h"
#include "rDataFilter.h"
#include "rError.h"
#include "rStrings.h"

#include <curl/curl.h>
#include <fstream>

using namespace rapio;
using namespace std;

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
  // Choose a decompressor or directly move buffer
  std::shared_ptr<DataFilter> f = Factory<DataFilter>::get(url.getSuffixLC(), "IOURL");

  if (f == nullptr) {
    buf = std::move(rawData);
  } else {
    // Try to decompress with the choosen decompressor
    std::vector<char> output;
    if (!f->apply(rawData, output)) {
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
    // Use curl to pull it.
    CurlConnection::read(url.toString(), buf);
  } else {
    // Local file ingest
    // ------------------------------------------------------------
    std::ifstream file(url.getPath());

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
