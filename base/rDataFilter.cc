#include "rDataFilter.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rBOOST.h"

BOOST_WRAP_PUSH
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/gzip.hpp>  // .gz files
#include <boost/iostreams/filter/bzip2.hpp> // .bz2 files
#include <boost/iostreams/filter/zlib.hpp>  // .z files
#if HAVE_LZMA
# include <boost/iostreams/filter/lzma.hpp> // .lzma files
#endif
BOOST_WRAP_POP

#include <fstream>

#if HAVE_SNAPPY
# include "snappy.h" // google .sz files
#endif

using namespace rapio;
using namespace std;

namespace bi = boost::iostreams;

namespace {
/** Common Utility to handle input/output file on a passed boost filtering_streambuf
 * We use the stream since I'm not even sure all the compressors have a common superclass,
 * BOOST tends to be template heavy.
 */
bool
applyBOOSTURL(const URL& infile, const URL& outfile, bi::filtering_streambuf<bi::output>& outbuf)
{
  std::ifstream filein(infile.toString(), std::ios_base::in | std::ios_base::binary);

  if (!filein.is_open()) {
    LogSevere("Unable to open input file: " << infile);
    return false;
  }

  std::ofstream fileout(outfile.toString(), std::ios_base::out | std::ios_base::binary);

  if (!fileout.is_open()) {
    LogSevere("Unable to open output file: " << outfile);
    return false;
  }

  try {
    outbuf.push(fileout);
    std::ostream out(&outbuf);
    out << filein.rdbuf();
    out.flush();  // Ensure all data is flushed out
    outbuf.pop(); // Finalize the compression/filtering process
  } catch (const std::exception& e) {
    LogSevere("From " << infile << " to " << outfile << " filter failed: " << e.what() << "\n");
    return false;
  }
  return true;
}

bool
applyBOOSTOstream(std::vector<char>& input, std::vector<char>& output, bi::filtering_ostream& os)
{
  // BOOST gz decompressor
  try{
    os.push(bi::back_inserter(output));
    bi::write(os, input.data(), input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (const bi::gzip_error& e) {
    LogSevere("Filter failure on stream: " << e.what() << "\n"); // Let caller notify?
    return false;
  }
  return true;
}

bool
applyBOOSTOstreamNew(
  std::vector<char>    & input,
  std::vector<char>    & output,
  bi::filtering_ostream& os,
  size_t               start_index = 0,
  size_t               length      = 0 //  (0 means to end)
)
{
  // Calculate the actual length to process
  if ((length == 0) || ((start_index + length) > input.size())) {
    length = input.size() - start_index;
  }

  // Check for valid range
  if (start_index >= input.size()) {
    // Handle error: start index is out of bounds
    return false;
  }

  // Get the pointer to the starting element
  const char * start_ptr = input.data() + start_index;

  try{
    os.push(bi::back_inserter(output));
    bi::write(os, start_ptr, length);
    os.flush();
  }catch (const bi::gzip_error& e) {
    LogSevere("Filter failure on stream: " << e.what() << "\n");
    return false;
  }
  return true;
}
}

void
DataFilter::introduce(const string & protocol,
  std::shared_ptr<DataFilter>      factory)
{
  Factory<DataFilter>::introduce(protocol, factory);
}

void
DataFilter::introduceSelf()
{
  GZIPDataFilter::introduceSelf();
  BZIP2DataFilter::introduceSelf();
  ZLIBDataFilter::introduceSelf();
  #if HAVE_LZMA
  LZMADataFilter::introduceSelf();
  #endif

  #if HAVE_SNAPPY
  SnappyDataFilter::introduceSelf();
  #endif
}

std::shared_ptr<DataFilter>
DataFilter::getDataFilter(const std::string& name)
{
  return (Factory<DataFilter>::get(name, "DataFilter"));
}

void
GZIPDataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<GZIPDataFilter>();

  DataFilter::introduce("gz", f);
}

bool
GZIPDataFilter::apply(std::vector<char>& input, std::vector<char>& output,
  size_t startIndex, size_t length)
{
  bi::filtering_ostream os;

  os.push(bi::gzip_decompressor());
  return (applyBOOSTOstream(input, output, os));
}

bool
GZIPDataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  bi::filtering_streambuf<bi::output> outbuf;
  // to do, settings, etc, right?
  const std::string level = params["gziplevel"];

  if (level.empty()) {
    outbuf.push(bi::gzip_compressor());
  } else {
    int ilevel = 0;
    try{
      ilevel = std::stoi(level);
      if ((ilevel < 0 ) || (ilevel > 9)) {
        ilevel = 0;
      }
    }catch (const std::exception& e)
    {
      ilevel = 0;
    }
    outbuf.push(bi::gzip_compressor(bi::gzip_params(ilevel) ));
  }
  return (applyBOOSTURL(infile, outfile, outbuf));
}

void
BZIP2DataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<BZIP2DataFilter>();

  DataFilter::introduce("bz2", f);
}

bool
BZIP2DataFilter::apply(std::vector<char>& input, std::vector<char>& output,
  size_t startIndex, size_t length)
{
  bi::filtering_ostream os;

  os.push(bi::bzip2_decompressor());
  // return (applyBOOSTOstream(input, output, os));
  return (applyBOOSTOstreamNew(input, output, os, startIndex, length));
}

bool
BZIP2DataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  bi::filtering_streambuf<bi::output> outbuf;

  // to do, settings, etc, right?
  outbuf.push(bi::bzip2_compressor());
  return (applyBOOSTURL(infile, outfile, outbuf));
}

void
ZLIBDataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<ZLIBDataFilter>();

  DataFilter::introduce("z", f);
}

bool
ZLIBDataFilter::apply(std::vector<char>& input, std::vector<char>& output,
  size_t startIndex, size_t length)
{
  bi::filtering_ostream os;

  os.push(bi::zlib_decompressor());
  return (applyBOOSTOstream(input, output, os));
}

bool
ZLIBDataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  bi::filtering_streambuf<bi::output> outbuf;

  // to do, settings, etc, right?
  outbuf.push(bi::zlib_compressor());
  return (applyBOOSTURL(infile, outfile, outbuf));
}

#if HAVE_LZMA
void
LZMADataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<LZMADataFilter>();

  DataFilter::introduce("lzma", f);
  DataFilter::introduce("xz", f);
}

bool
LZMADataFilter::apply(std::vector<char>& input, std::vector<char>& output,
  size_t startIndex, size_t length)
{
  bi::filtering_ostream os;

  os.push(bi::lzma_decompressor());
  return (applyBOOSTOstream(input, output, os));
}

bool
LZMADataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  bi::filtering_streambuf<bi::output> outbuf;

  outbuf.push(bi::lzma_compressor());
  return (applyBOOSTURL(infile, outfile, outbuf));
}

#endif // if HAVE_LZMA

#if HAVE_SNAPPY
void
SnappyDataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<SnappyDataFilter>();

  DataFilter::introduce("sz", f);
}

bool
SnappyDataFilter::apply(std::vector<char>& input, std::vector<char>& output,
  size_t startIndex, size_t length)
{
  try {
    // Get the size of the uncompressed data for output buffer
    size_t outputsize;
    bool valid = snappy::GetUncompressedLength(input.data(), input.size(), &outputsize);
    if (!valid) {
      LogSevere("Snappy data format not recognized.\n");
      return false;
    }

    output.resize(outputsize);

    // Attempt to decompress the data
    snappy::RawUncompress(input.data(), input.size(), output.data());

    // Check if the decompression was successful by ensuring the output is valid
    if (output.size() != outputsize) {
      LogSevere("Decompressed size does not match expected size.\n");
      return false;
    }

    return true; // Return true if everything succeeded
  } catch (const std::exception& e) {
    LogSevere("Snappy filter failed: " << e.what() << "\n");
    return false;
  }
}

bool
SnappyDataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  std::vector<char> bufin;
  std::vector<char> bufout;

  // Read input file into buffer
  if (IOURL::read(infile, bufin) > 0) {
    try {
      // Determine maximum output size
      size_t maxOutputSize = snappy::MaxCompressedLength(bufin.size());
      bufout.resize(maxOutputSize);

      // Compress the data
      size_t compressedSize = 0;
      snappy::RawCompress(&bufin[0], bufin.size(), &bufout[0], &compressedSize);

      // Open output file for writing compressed data
      std::ofstream fileout(outfile.toString(), std::ios_base::out | std::ios_base::binary);
      if (!fileout.is_open()) {
        LogSevere("Unable to open output file: " << outfile);
        return false;
      }

      // Write only the compressed size to file
      fileout.write(&bufout[0], compressedSize);
      fileout.close();

      return true;
    } catch (const std::exception& e) {
      LogSevere("Snappy compress from " << infile << " to " << outfile << " failed: " << e.what());
      return false;
    }
  } else {
    LogSevere("Unable to read input file: " << infile);
    return false;
  }
} // SnappyDataFilter::applyURL

#endif // if HAVE_SNAPPY
