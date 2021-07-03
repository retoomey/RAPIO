#include "rDataFilter.h"

#include "rFactory.h"
#include "rIOURL.h"

#include <fstream>

// BOOST compression
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/gzip.hpp>  // .gz files
#include <boost/iostreams/filter/bzip2.hpp> // .bz2 files
#include <boost/iostreams/filter/zlib.hpp>  // .z files
#include <boost/iostreams/filter/lzma.hpp>  // .lzma files

#if RAPIO_USE_SNAPPY == 1
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
  // Copy from infile to outfile, passing through gzip
  try{
    std::ifstream filein(infile.toString(), std::ios_base::in | std::ios_base::binary);
    std::ofstream fileout(outfile.toString(), std::ios_base::out | std::ios_base::binary);

    // Add extra stuff to the passed in stream
    outbuf.push(fileout);
    // Convert streambuf to ostream
    std::ostream out(&outbuf);
    out << filein.rdbuf();
    bi::close(outbuf);
    fileout.close();
    return true;
  }catch (const std::exception& e) {
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
    bi::write(os, &input[0], input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (const bi::gzip_error& e) {
    LogSevere("Filter failure on stream: " << e.what() << "\n"); // Let caller notify?
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
  LZMADataFilter::introduceSelf();

  #if RAPIO_USE_SNAPPY == 1
  // Playing with snappy some.  The downside it that for whatever
  // reason that while snappy-devel tends to exist on modern linux,
  // a basic snappy decompress binary does not seem to, so this makes this
  // format inconvenient for regular users. We could probably build a simple
  // command line tool.
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
GZIPDataFilter::apply(std::vector<char>& input, std::vector<char>& output)
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
BZIP2DataFilter::apply(std::vector<char>& input, std::vector<char>& output)
{
  bi::filtering_ostream os;
  os.push(bi::bzip2_decompressor());
  return (applyBOOSTOstream(input, output, os));
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
ZLIBDataFilter::apply(std::vector<char>& input, std::vector<char>& output)
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

void
LZMADataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<LZMADataFilter>();
  DataFilter::introduce("lzma", f);
  DataFilter::introduce("xz", f);
}

bool
LZMADataFilter::apply(std::vector<char>& input, std::vector<char>& output)
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

#if RAPIO_USE_SNAPPY == 1
void
SnappyDataFilter::introduceSelf()
{
  std::shared_ptr<DataFilter> f = std::make_shared<SnappyDataFilter>();
  DataFilter::introduce("sz", f);
}

bool
SnappyDataFilter::apply(std::vector<char>& input, std::vector<char>& output)
{
  try{
    // Get size of the uncompressed data for output buffer
    size_t outputsize;
    bool valid = snappy::GetUncompressedLength(&input[0], input.size(), &outputsize);
    if (!valid) {
      LogSevere("Snappy data format not recognized.\n");
      return false;
    }
    output.resize(outputsize);
    snappy::RawUncompress(&input[0], input.size(), &output[0]);

    return false;
  }catch (const std::exception& e) {
    LogSevere("Snappy filter failed: " << e.what() << "\n");
    return false;
  }
}

bool
SnappyDataFilter::applyURL(const URL& infile, const URL& outfile,
  std::map<std::string, std::string> &params)
{
  // Copy from infile to outfile, passing through gzip
  try{
    std::ifstream filein(infile.toString(), std::ios_base::in | std::ios_base::binary);
    std::ofstream fileout(outfile.toString(), std::ios_base::out | std::ios_base::binary);

    std::vector<char> bufin;
    std::vector<char> bufout;
    if (IOURL::read(infile, bufin) > 0) {
      // Get the max output vector size...
      size_t outputsize = snappy::MaxCompressedLength(bufin.size());
      bufout.resize(outputsize);
      snappy::RawCompress(&bufin[0], bufin.size(), &bufout[0], &outputsize);
      //      LogSevere("Read in file to buffer..in size " << bufin.size() << " and out is " << outputsize << "\n");

      fileout.write(&bufout[0], bufout.size());
      fileout.close();
    }

    return true;
  }catch (const std::exception& e) {
    LogSevere("Snappy compress from " << infile << " to " << outfile << " failed.\n");
    return false;
  }
  return true;
}

#endif // if RAPIO_USE_SNAPPY == 1
