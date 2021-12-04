#pragma once

#include <rProcessTimer.h>
#include <rError.h>
#include <zlib.h>

// Most of these currently are meant to be called externally...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// included in .cc files that need binary IO capability
namespace {
/** Z compress a single vector column.  This is actually pretty
 * efficient since all items in the column share a number domain */
template <typename T> bool
compressZLIB(
  std::vector<T>            & input, // Input vector of stuff to compress
  std::vector<unsigned char>& out_data)
{
  size_t in_data_size = input.size() * sizeof(T);

  std::vector<unsigned char> buffer;

  const size_t BUFSIZE = 128 * 1024;
  unsigned char temp_buffer[BUFSIZE];

  z_stream strm;

  strm.zalloc    = 0;
  strm.zfree     = 0;
  strm.next_in   = reinterpret_cast<unsigned char *>(&input[0]);
  strm.avail_in  = in_data_size;
  strm.next_out  = temp_buffer;
  strm.avail_out = BUFSIZE;

  deflateInit(&strm, Z_BEST_COMPRESSION);

  while (strm.avail_in != 0) {
    int res = deflate(&strm, Z_NO_FLUSH);

    // assert(res == Z_OK);
    if (res != Z_OK) { return (false); }

    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out  = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
  }

  int deflate_res = Z_OK;

  while (deflate_res == Z_OK) {
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
      strm.next_out  = temp_buffer;
      strm.avail_out = BUFSIZE;
    }
    deflate_res = deflate(&strm, Z_FINISH);
  }

  // assert(deflate_res == Z_STREAM_END);
  if (deflate_res != Z_STREAM_END) { return (false); } buffer.insert(
    buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
  deflateEnd(&strm);

  out_data.swap(buffer);
  return (true);
} // compressZLIB

/** Uncompress a vector column that was compressed with the compressZLIB routine
 */
template <typename T> bool
uncompressZLIB(
  std::vector<unsigned char>& input,
  std::vector<T>            & out_data,
  size_t                    knownEndSize) // Hope you stored this it's the
                                          // T.size() not bytes
{
  out_data.resize(knownEndSize);
  uLongf is = input.size() * sizeof(unsigned char);
  uLongf os = knownEndSize * sizeof(T);
  int error =
    uncompress((unsigned char *) &out_data[0],
      &os,
      (unsigned char *) (&input[0]),
      is);

  if (error != Z_OK) {
    LogSevere("ZLIB ERROR:" << error << "\n");
  }
  return (error == Z_OK);
}

/** Write a string (not bothering to compress it) */
void
write_string16(const std::string& s, FILE * fp)
{
  size_t lenlong = s.size();

  if (lenlong > 65535) { lenlong = 65535; }
  unsigned short len = (unsigned short) (lenlong);

  fwrite(&len, sizeof(len), 1, fp);
  fwrite(s.c_str(), sizeof(char), len, fp);
}

/** Write a string with max size of 255 (8 bits for length)*/
void
write_string8(const std::string& s, FILE * fp)
{
  size_t lenlong = s.size();

  if (lenlong > 255) { lenlong = 255; }
  unsigned char len = (unsigned char) (lenlong);

  fwrite(&len, sizeof(len), 1, fp);
  fwrite(s.c_str(), sizeof(char), len, fp);
}

/** Write a generic object with a fixed size  */
template <typename T> void
write_type(const T& s, FILE * fp)
{
  // LogInfo("Write type size is " << sizeof(T) << "\n");
  fwrite(&s, sizeof(T), 1, fp);
}

/** Write a vector.  Make sure it matches the read vector */
template <class T>
void
write_vector(const std::string& label,
  std::vector<T>              & vec,
  FILE *                      fp,
  int                         mode = 1 /* ZLIB default */)
{
  const size_t len = vec.size();

  // Don't try compression with zero length vectors...
  if (len == 0) { mode = 0; }

  /** Store depending on the storage mode */

  // Could enum these but only used here and the read below
  // 0 -- Normal 1 -- ZLIB ... add more if needed and enum if needed.

  if (mode == 1) { // ZLIB
    std::vector<unsigned char> compressed;
    rapio::ProcessTimer timer("ZLIB compression " + label + " took:");
    bool success = compressZLIB<T>(vec, compressed);

    if (success) {
      const size_t csize     = compressed.size();
      const size_t cBYTESize = csize * sizeof(unsigned char);
      const size_t vBYTESize = len * sizeof(T);

      if (cBYTESize < vBYTESize) {
        // ------------------------------------------------------------
        // ZLIB -------------------------------------------------------

        /** Store the mode of storage. Don't move out, we change this mode on
         * failure */
        fwrite(&mode, sizeof(mode), 1, fp);

        /** Store the full length of the vector (units of T) */
        fwrite(&len, sizeof(len), 1, fp);

        /** Store the compressed length of the vector (units of unsigned char)
         */
        fwrite(&csize, sizeof(csize), 1, fp);

        /** Store the compressed vector */
        fwrite(&(compressed[0]), sizeof(compressed[0]), csize, fp);

        // Print out the saving percentage for now...
        const float percent =
          ((float) (vBYTESize - cBYTESize) / (float) (vBYTESize)) * 100.0;
        LogInfo("ZLIB SAVED: " << label << " size percent " << percent << "% "
                               << vBYTESize << " --> " << cBYTESize << "\n");
      } else {
        LogInfo("ZLIB didn't save any space " << cBYTESize << " < " << vBYTESize
                                              << " skipping compression of vector\n");
        mode = 0;
      }

      // -----------------------------------------------------------
    } else {
      LogSevere("ZLIB compression failure, failing back to uncompressed column\n");
      mode = 0;
    }
  }

  /** Normal uncompressed data */
  if (mode == 0) { // Normal mode
    // ------------------------------------------------------------
    // Normal -----------------------------------------------------

    /** Store the mode of storage. */
    fwrite(&mode, sizeof(mode), 1, fp);

    /** Store the full length of the vector */
    fwrite(&len, sizeof(len), 1, fp);

    /** Store the vector */
    fwrite(&(vec[0]), sizeof(vec[0]), len, fp);

    // ------------------------------------------------------------
  }
} // write_vector

/** Read a vector */
template <class T>
void
read_vector(std::vector<T>& vec, FILE * fp)
{
  /** Read the storage mode */
  int mode;

  fread(&mode, sizeof(mode), 1, fp);

  // LogSevere("Compression mode is " << mode << "\n");

  /** Read the full length of the vector */
  size_t len;

  fread(&len, sizeof(len), 1, fp);

  // LogSevere("Length of vector is  " << len << "units ("<<len*sizeof(T) <<"
  // bytes)\n");

  if (mode == 1) { // ZLIB
    // ------------------------------------------------------------
    // ZLIB -------------------------------------------------------

    /** Read the compressed length of the vector */
    size_t csize;
    fread(&csize, sizeof(csize), 1, fp);

    // LogSevere("Length of COMPRESSED vector is (" << csize << " bytes)\n");

    /** Read the compressed vector */
    std::vector<unsigned char> compressed;
    compressed.resize(csize);
    fread(&(compressed[0]), sizeof(compressed[0]), csize, fp);

    /** ZLIB uncompress */
    rapio::ProcessTimer timer("ZLIB UNCOMPRESSION took:");

    if (!uncompressZLIB<T>(compressed, vec, len)) {
      LogSevere("ZLIB uncompress failure. Different .raw format file?\n");
      vec.clear();
    }

    // -----------------------------------------------------------
  } else if (mode == 0) { // Normal
    vec.resize(len);
    fread(&(vec[0]), sizeof(vec[0]), len, fp);
  } else {
    LogSevere(
      "Storage mode unrecognized: " << mode << ", can't handle data file.\n");
    LogSevere(
      "This file is too probably too new or corrupted, you probably need to update your build.\n");
    vec.clear(); // Return nothing...
  }
} // read_vector

/** private read string code */
void inline
rawReadString(std::string& s, size_t len, FILE * fp)
{
  s = "";

  for (size_t i = 0; i < len; ++i) {
    char c;
    fread(&c, sizeof(char), 1, fp);
    s = s + c;
  }
}

/** Read a string with a two byte length, default maxlen of 10000 */
void
read_string16(std::string& s, FILE * fp, size_t maxlen = 10000)
{
  unsigned short len;

  fread(&len, sizeof(len), 1, fp);

  if (len > maxlen) { return; }

  rawReadString(s, len, fp);
}

/** Write a string with max size of 255 (8 bits for length)*/
void
read_string8(std::string& s, FILE * fp)
{
  unsigned char len;

  fread(&len, sizeof(len), 1, fp);
  rawReadString(s, len, fp);
}

/** Read a generic type with a fixed size (this doesn't work for dynamic sized
 * things
 * like strings or vectors of objects (it will write the address) */
template <typename T> void
read_type(T& s, FILE * fp)
{
  fread(&s, sizeof(T), 1, fp);
}

/*
 * // Toomey: Simple run-length encoding of data values. There's a lot of
 *    repeating patterns in these
 * // values especially the X and Y vectors that we can compress efficiently.
 * // Mine was only better with X/Y than ZLIB, others it beat me pretty easy,
 *    lol.  Still maybe
 * // I'll find a use for this code someday so I'll keep it...
 * template <typename K, typename T> void compressRLE(
 *     const std::vector<T>& input,  //Input array of a type
 *     std::vector<T>& output, //Output array of that type
 *     std::vector<K>& counts) //The runlength index values (must be countable)
 * {
 *    const size_t s = input.size();
 *    K count = 0;
 *    T lastValue = 0;
 *
 *    for(size_t i=0; i<s; i++){ // Go through all the data...
 *      const T value = input[i];
 *
 *      // There's no old data to write..skip to next
 *      if (i == 0){ count++; lastValue = value; continue; }
 *
 *      // Did the value change?
 *      if (value != lastValue){
 *        // Push back the previous run
 *        counts.push_back(count);
 *        output.push_back(lastValue);
 *        // New run of count one...
 *        count = 1; lastValue = value;
 *      }else{
 *        // it's the same, add to count.
 *        count++;
 *      }
 *      if (i == s-1){ // On last one, push back what we have...
 *        counts.push_back(count);
 *        output.push_back(lastValue);
 *      }
 *    }
 * }
 *
 * template <typename K, typename T> void uncompress(
 *    const std::vector<T>& input,  // Input array of a type
 *    std::vector<T>& output, // Output array of that type
 *    std::vector<K>& counts) // The runlength index values (must be countable)
 * {
 *   const size_t s = input.size();
 *   for(size_t i=0; i< s; i++){
 *      const K count = counts[i];
 *      const T value = input[i];
 *      for(size_t j=0; j<count; j++){
 *        output.push_back(value);
 *      }
 *   }
 * }
 */
}
#pragma GCC diagnostic pop
