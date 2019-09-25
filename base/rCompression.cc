#include "rCompression.h"
#include "rError.h"

// BOOST compression
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/gzip.hpp>  // .gz files
#include <boost/iostreams/filter/bzip2.hpp> // .bz2 files
#include <boost/iostreams/filter/zlib.hpp>  // .z files
#include <boost/iostreams/filter/lzma.hpp>  // .lzma files

using namespace rapio;

// FIXME: might be able to group common code, but boost makes it difficult to
// figure out common superclasses...looks like gzip_decompressor, bzip_decompressor,
// etc. are unique structures
bool
Compression::uncompressGzip(std::vector<char>& input, std::vector<char>& output)
{
  // BOOST gz decompressor
  try{
    boost::iostreams::filtering_ostream os;
    os.push(boost::iostreams::gzip_decompressor());
    os.push(boost::iostreams::back_inserter(output));
    boost::iostreams::write(os, &input[0], input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (boost::iostreams::gzip_error e) {
    LogSevere("GZIP decompress failed.\n"); // Let caller notify?
    return false;
  }
  return true;
}

bool
Compression::uncompressBzip2(std::vector<char>& input, std::vector<char>& output)
{
  // BOOST bz2 decompressor
  try{
    boost::iostreams::filtering_ostream os;
    os.push(boost::iostreams::bzip2_decompressor());
    os.push(boost::iostreams::back_inserter(output));
    boost::iostreams::write(os, &input[0], input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (boost::iostreams::bzip2_error e) {
    LogSevere("BZIP2 decompress failed.\n"); // Let caller notify?
    return false;
  }
  return true;
}

bool
Compression::uncompressZlib(std::vector<char>& input, std::vector<char>& output)
{
  // BOOST zlib decompressor
  try{
    boost::iostreams::filtering_ostream os;
    os.push(boost::iostreams::zlib_decompressor());
    os.push(boost::iostreams::back_inserter(output));
    boost::iostreams::write(os, &input[0], input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (boost::iostreams::zlib_error e) {
    LogSevere("ZLIB decompress failed.\n"); // Let caller notify?
    return false;
  }
  return true;
}

bool
Compression::uncompressLzma(std::vector<char>& input, std::vector<char>& output)
{
  // BOOST lzma decompressor
  try{
    boost::iostreams::filtering_ostream os;
    os.push(boost::iostreams::lzma_decompressor());
    os.push(boost::iostreams::back_inserter(output));
    boost::iostreams::write(os, &input[0], input.size());
    os.flush(); // _HAVE_ to do this or data's clipped, not online anywhere lol
  }catch (boost::iostreams::lzma_error e) {
    LogSevere("LZMA decompress failed.\n"); // Let caller notify?
    return false;
  }
  return true;
}
