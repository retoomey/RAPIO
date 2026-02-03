// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rBinaryIO.h"

#include <fstream>
#include <string>
#include <vector>
#include <cstdio> // For std::remove

using namespace rapio;
using namespace std;

// Define test files
const char * FILE_PATH = "/tmp/binary_test.dat";
const char * GZIP_PATH = "/tmp/gzip_test.gz";

// 1. Define Test Data
const int TEST_INT              = 123456;
const short TEST_SHORT          = -1024;
const float TEST_FLOAT          = 3.14159f;
const char TEST_CHAR            = 'A';
const string TEST_STRING        = "Hello World!";
const size_t TEST_STRING_LEN    = 20; // Fixed length for write/read string
const vector<float> TEST_VECTOR = { 10.5f, 20.5f, 30.5f, 40.5f };
const size_t TEST_VECTOR_SIZE   = TEST_VECTOR.size() * sizeof(float);

// 2. Helper function to perform the writing sequence
void
write_test_data(StreamBuffer& g)
{
  // A. Write standard types
  g.writeInt(TEST_INT);
  g.writeShort(TEST_SHORT);
  g.writeChar(TEST_CHAR);
  g.writeFloat(TEST_FLOAT);

  // B. Write fixed-length string (padding/clipping must work)
  g.writeString(TEST_STRING, TEST_STRING_LEN);

  // C. Write vector of raw bytes
  g.writeVector(TEST_VECTOR.data(), TEST_VECTOR_SIZE);
}

// 3. Helper function to perform the reading sequence and assertions
void
read_test_data(StreamBuffer& g)
{
  // A. Read standard types
  BOOST_CHECK_EQUAL(g.readInt(), TEST_INT);
  BOOST_CHECK_EQUAL(g.readShort(), TEST_SHORT);
  BOOST_CHECK_EQUAL(g.readChar(), TEST_CHAR);
  // Use a tolerance for floating point checks
  BOOST_CHECK_CLOSE(g.readFloat(), TEST_FLOAT, 0.0001f);

  // B. Read fixed-length string
  string readStr = g.readString(TEST_STRING_LEN);

  BOOST_CHECK_EQUAL(readStr, TEST_STRING);
  BOOST_CHECK_EQUAL(readStr.size(), TEST_STRING.size());

  // C. Read vector of raw bytes
  vector<float> readVector(TEST_VECTOR.size());

  g.readVector(readVector.data(), TEST_VECTOR_SIZE);

  for (size_t i = 0; i < TEST_VECTOR.size(); ++i) {
    BOOST_CHECK_CLOSE(readVector[i], TEST_VECTOR[i], 0.0001f);
  }

  // Seek back to start and read the first int again
  BOOST_CHECK(g.seek(0));
  BOOST_CHECK_EQUAL(g.readInt(), TEST_INT);
}

BOOST_AUTO_TEST_SUITE(STREAM_BUFFER_TESTS)

BOOST_AUTO_TEST_CASE(STREAMBUFFER_ALL_FUNCTIONS)
{
  // --- 1. MemoryStreamBuffer Test ---

  // Create an initial memory buffer (empty, will grow via ensureAvailable)
  std::vector<char> buf;
  MemoryStreamBuffer memBuf(std::move(buf));

  BOOST_TEST_MESSAGE("Testing MemoryStreamBuffer (Write & Read)");

  // Write sequence
  BOOST_CHECK_NO_THROW(write_test_data(memBuf));

  // Read sequence
  memBuf.seek(0); // reset for reading
  BOOST_CHECK_NO_THROW(read_test_data(memBuf));

  // --- 2. FileStreamBuffer Test ---

  BOOST_TEST_MESSAGE("Testing FileStreamBuffer (Write & Read)");

  // --- Write ---
  FILE * writeFile = std::fopen(FILE_PATH, "wb");

  BOOST_REQUIRE(writeFile != nullptr);
  FileStreamBuffer fileBufW(writeFile);

  fileBufW.setDataLittleEndian();

  BOOST_CHECK_NO_THROW(write_test_data(fileBufW));
  fclose(writeFile);

  // --- Read back ---
  FILE * readFile = std::fopen(FILE_PATH, "rb");

  BOOST_REQUIRE(readFile != nullptr);
  FileStreamBuffer fileBufR(readFile);

  fileBufR.setDataLittleEndian();

  BOOST_CHECK_NO_THROW(read_test_data(fileBufR));
  fclose(readFile);

  // Remove temp file
  std::remove(FILE_PATH);

  // --- 3. GzipFileStreamBuffer Test ---

  BOOST_TEST_MESSAGE("Testing GzipFileStreamBuffer (Write & Read)");

  // --- Write ---
  gzFile writeGzFile = gzopen(GZIP_PATH, "wb");

  BOOST_REQUIRE(writeGzFile != nullptr);
  GzipFileStreamBuffer gzipBufW(writeGzFile);

  gzipBufW.setDataLittleEndian();

  BOOST_CHECK_NO_THROW(write_test_data(gzipBufW));
  gzclose(writeGzFile);

  // --- Read ---
  gzFile readGzFile = gzopen(GZIP_PATH, "rb");

  BOOST_REQUIRE(readGzFile != nullptr);
  GzipFileStreamBuffer gzipBufR(readGzFile);

  BOOST_CHECK_NO_THROW(read_test_data(gzipBufR));
  gzclose(readGzFile);

  // Clean up temporary file
  std::remove(GZIP_PATH);
}

BOOST_AUTO_TEST_SUITE_END()
