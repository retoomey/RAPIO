#include "rCompression.h"

#include "rByteOrder.h"
#include "rBuffer.h"
#include "rError.h"

#include <zlib.h>
#include <stdio.h> // needed by bzlib.h!!!
#include <bzlib.h>
#include <string.h>
#include <cassert>

/*
 *
 * Much of the uncompressGZ is taken gzio.c in the zlib library,
 * and that portion is copyrighted by Jean-loup Gailly and Mark Adler
 * It is used here in accordance with the zlib copyright:
 *
 *  Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *  claim that you wrote the original software. If you use this software
 *  in a product, an acknowledgment in the product documentation would be
 *  appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *  misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source distribution.
 *
 */


using namespace rapio;

namespace {
#define ASCII_FLAG  0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC    0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD 0x04 /* bit 2 set: extra field present */
#define ORIG_NAME   0x08 /* bit 3 set: original file name present */
#define COMMENT     0x10 /* bit 4 set: file comment present */
#define RESERVED    0xE0 /* bits 5..7: reserved */

size_t
guessUncomprLength(size_t comprLen)
{
  const size_t minBufferLen = 1000000; // 1 MB
  const size_t maxBufferLen = 5000000; // 5 MB to start

  size_t guessedLen = comprLen * 10; // at least 10x compression

  if (guessedLen < minBufferLen) {
    return (minBufferLen);
  }

  if (guessedLen > maxBufferLen) {
    return (maxBufferLen);
  }

  return (guessedLen);
}

void
putLong(char * inhere, unsigned int val)
{
  static ByteOrder os(false);
  char * fromhere = (char *) &val;

  os.swapIfNeeded(fromhere, 4);

  for (int i = 0; i < 4; ++i) {
    inhere[i] = fromhere[i];
  }
}

unsigned int
getLong(const char * from_here)
{
  char val[4];

  for (int i = 0; i < 4; ++i) {
    val[i] = from_here[i];
  }

  static ByteOrder os(false);
  os.swapIfNeeded(val, 4);

  unsigned int retVal;

  for (int i = 0; i < 4; ++i) {
    *(((char *) &retVal) + i) = val[i];
  }
  return (retVal);
}

int
gobbleByte(const Bytef *& fromHere)
{
  int c = *fromHere;

  fromHere++;
  return (c);
}

int gz_magic[2] = { 0x1f, 0x8b }; // gzip magic header

void
addGzipHeader(char * startHere)
{
  // the gzip header, stolen from gzio.c in the zlib
  static int OS_CODE = 0x03; // Unix

  char oldChar = startHere[10];

  sprintf(startHere,
    "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
    Z_DEFLATED, 0 /*flags*/, 0, 0, 0, 0 /*time*/, 0 /*xflags*/, OS_CODE);

  startHere[10] = oldChar; // 'cos sprintf puts in a '\0'
}

bool
eofs(const Bytef * curr, const Bytef * start, size_t len)
{
  size_t traversed = curr - start;

  if (traversed < len) {
    return (false);
  }

  return (true);
}

int
checkGzipHeader(const Bytef * start, size_t in_len)
{
  // stolen from gzip.c
  if (in_len < 10) { return (-1); }

  uInt len;
  int c;

  const Bytef * data = start;

  for (len = 0; len < 2; ++len) {
    c = gobbleByte(data);

    if (c != gz_magic[len]) { return (-2); }
  }

  int method = gobbleByte(data);
  int flags  = gobbleByte(data);

  if ((method != Z_DEFLATED) || ((flags & RESERVED) != 0)) {
    return (-3);
  }

  // discard time, xflags and OS code
  data += 6;

  if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
    len  = (uInt) gobbleByte(data);
    len += ((uInt) gobbleByte(data)) << 8;

    /* len is garbage if EOF but the loop below will quit anyway */
    while (len-- != 0 && gobbleByte(data) && !eofs(data, start, in_len)) { }
  }

  if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
    while ((c = gobbleByte(data)) != 0 && !eofs(data, start, in_len)) { }
  }

  if ((flags & COMMENT) != 0) { /* skip the .gz file comment */
    while ((c = gobbleByte(data)) != 0 && !eofs(data, start, in_len)) { }
  }

  if ((flags & HEAD_CRC) != 0) { /* skip the header crc */
    while ((c = gobbleByte(data)) != 0 && !eofs(data, start, in_len)) { }
  }

  if (eofs(data, start, in_len)) { return (-3); }

  // return the header length
  return (data - start);
} // checkGzipHeader

void
addGzipFooter(char * startHere, const Bytef * uncomprData,
  size_t uncomprLen, size_t comprLen)
{
  // add the gzip bookkeeping information
  unsigned int corr_code = crc32(0L, Z_NULL, 0);

  corr_code = crc32(corr_code,
      uncomprData,
      uncomprLen);

  putLong(startHere, corr_code);
  putLong(startHere + 4, uncomprLen);
}
}


Buffer
Compression::uncompressBZ2(const Buffer& input)
{
  unsigned final_len = guessUncomprLength(input.getLength());
  Buffer ret_buffer(final_len);

  while (true) {
    char * destination = ret_buffer.begin();
    char * source      = const_cast<char *>(input.begin());

    /*
     *  If you have trouble with this function being unresolved, you may have an
     *  old copy of the bzip2 library.  Either update your bzip2 installation,
     * or
     *  alternatively, you could remove the BZ2 prefix from the function, making
     * it
     *  read:
     *
     *  int ret = bzBuffToBuffDecompress
     *
     */

    int ret = BZ2_bzBuffToBuffDecompress
      (
      destination, &(final_len), source, input.data().size(),
      0, //  keep it fast
      0  //  and silent
      );

    if (ret == BZ_OK) {
      //  remove the extra bytes
      ret_buffer.data().resize(final_len);
      return (ret_buffer);
    }

    if (ret != BZ_OUTBUFF_FULL) {
      LogSevere("compressBZ2: Can't handle error: " << ret
                                                    << "\n");
      return (Buffer());
    }

    //  if OUTBUFF_FULL, we will double memory size and try again
    final_len = ret_buffer.data().size() * 2;
    ret_buffer.data().resize(final_len);
  }

  //  never comes here
  assert(false);
  return (Buffer());
} // Compression::uncompressBZ2

Buffer
Compression::uncompressZlib(const Buffer& input)
{
  unsigned long final_len =
    guessUncomprLength(input.getLength());

  Buffer ret_buffer(final_len);

  while (true) {
    Bytef * destination  = (Bytef *) (ret_buffer.begin());
    const Bytef * source = (const Bytef *) (input.begin());
    int sourceLen        = input.data().size();

    int ret = uncompress
      (
      destination, &(final_len), source, sourceLen
      );

    if (ret == Z_OK) {
      //  remove the extra bytes
      ret_buffer.data().resize(final_len);
      return (ret_buffer);
    }

    if (ret == Z_DATA_ERROR) {
      LogSevere("Data not in zlib format. Returning original.\n");
      return (input);
    }

    if (ret == Z_BUF_ERROR) {
      final_len = ret_buffer.data().size() * 2;
      ret_buffer.data().resize(final_len);
    }

    LogSevere("Compression: can not handle gzip error: "
      << ret << "\n");
    return (Buffer());
  }

  //  never comes here
  assert(false);
  return (Buffer());
} // Compression::uncompressZlib

Buffer
Compression::uncompressGZ(const Buffer& input)
{
  int headerSize = checkGzipHeader((const Byte *) input.begin(),
      input.data().size());

  if (headerSize <= 0) {
    LogInfo("Not in gzip format. Returning original.\n");
    return (input);
  }

  int uncomprLen = getLong(&(input.data()[input.data().size() - 4]));

  if (uncomprLen <= 0) {
    LogSevere(
      "WARNING! The gzip file appears to be truncated. Returning an empty buffer.\n");
    return (Buffer());
  }

  Buffer result(uncomprLen);

  int ret = uncompressZlibNoHeader(input, result, headerSize);

  if (ret == Z_OK) { return (result); }

  if (ret == Z_DATA_ERROR) {
    LogSevere("Reading in gzip form failed. Returning original.\n");
    return (input);
  }

  if (ret == Z_BUF_ERROR) {
    LogSevere("Invalid gzip length: " << uncomprLen << "\n");
    return (Buffer());
  }

  LogSevere("Compression: can not handle gzip error: "
    << ret << "\n");
  return (Buffer());
} // Compression::uncompressGZ

int
Compression::uncompressZlibNoHeader(
  const Buffer& input,
  Buffer      & result,
  size_t      headerSize)
{
  // stolen from zlib, using inflateInit2 to say there's not zlib header
  z_stream stream;
  int err;

  stream.next_in   = (Bytef *) (input.begin() + headerSize);
  stream.avail_in  = (uInt) (input.data().size() - headerSize);
  stream.next_out  = (Bytef *) result.begin();
  stream.avail_out = result.data().size();

  stream.zalloc = (alloc_func) 0;
  stream.zfree  = (free_func) 0;
  stream.opaque = (voidpf) 0;

  err = inflateInit2(&stream, -15); // -ve --> no zlib header

  if (err != Z_OK) { return (err); }

  err = inflate(&stream, Z_FINISH);

  if (err != Z_STREAM_END) {
    inflateEnd(&stream);
    return (err == Z_OK ? Z_BUF_ERROR : err);
  }

  if (stream.total_out != result.data().size()) {
    LogSevere("Warning! Compression length unexpected.\n");
    return (Z_DATA_ERROR);
  }

  err = inflateEnd(&stream);
  return (err);
} // Compression::uncompressZlibNoHeader

Buffer
Compression::compressGZ(const Buffer& uncompr, float factor,
  float maxfactor)
{
  // header is 10 bytes, but the last two bytes are the CRC-16
  return (compressGZ(uncompr, factor, maxfactor, 8));
}

Buffer
Compression::compressZlib(const Buffer& uncompr, float factor,
  float maxfactor)
{
  return (compressGZ(uncompr, factor, maxfactor, 0));
}

Buffer
Compression::compressGZ(const Buffer& uncompr, float factor,
  float maxfactor,
  size_t HeaderSize)
{
  // from zlib manual -- size*1.1 + 10 bytes is enough
  Buffer compr(size_t(uncompr.data().size() * factor + HeaderSize + 50));

  unsigned long compr_bytes = compr.data().size() - HeaderSize;
  int ret_code =
    compress2(
    (Bytef *) (compr.begin() + HeaderSize),
    &compr_bytes,
    (const Bytef *) (uncompr.begin()),
    uncompr.data().size(),
    Z_DEFAULT_COMPRESSION
    );

  if (ret_code == Z_OK) {
    // resize it
    if (HeaderSize == 0) {
      // zlib
      compr.data().resize(compr_bytes);
    } else {
      // gzip
      compr.data().resize(compr_bytes + 12);
      addGzipHeader(compr.begin()); // writes 10 bytes
      addGzipFooter(&(compr.data()[compr.data().size() - 8]),
        (const Bytef *) uncompr.begin(),
        uncompr.data().size(),
        compr_bytes); // writes 8 bytes
    }

    return (compr);
  }

  // errors ...
  if (ret_code == Z_BUF_ERROR) {
    if (factor < maxfactor) {
      LogImpInfo
        ("Insufficient compression, using 10% more memory.\n");
      return (compressGZ(uncompr, factor * 1.1, maxfactor));
    }
  }

  LogSevere("Returning uncompressed (not gzipped) output.\n");
  return (uncompr);
} // Compression::compressGZ

Buffer
Compression::compressBZ2(const Buffer& uncompr, float factor,
  float maxfactor)
{
  Buffer compr(size_t(uncompr.data().size() * factor));
  unsigned int compr_bytes = compr.data().size();
  int ret_code =
    BZ2_bzBuffToBuffCompress(
    compr.begin(),
    &compr_bytes,
    const_cast<char *>(uncompr.begin()),
    uncompr.data().size(),
    9, // blocksize100k
    0, // verbose, 0-silent, 4-verbose
    0  // default
    );

  if (ret_code == BZ_OK) {
    compr.data().resize(compr_bytes);
    return (compr);
  }

  // errors ...
  if (ret_code == BZ_OUTBUFF_FULL) {
    if (factor < maxfactor) {
      LogImpInfo
        ("Insufficient compression, using 10% more memory.\n");
      return (compressBZ2(uncompr, factor * 1.1, maxfactor));
    }
  }

  LogSevere("Writing uncompressed (not block encoded) output.\n");
  return (uncompr);
}
