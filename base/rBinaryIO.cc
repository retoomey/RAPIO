#include "rBinaryIO.h"

#include <rError.h>
#include <rOS.h> // FIXME: temp

#include <algorithm>

using namespace rapio;
using namespace std;

bool
FileStreamBuffer::seek(size_t position)
{
  ERRNO(fseek(file, position, SEEK_SET));
  marker = position;
  return true;
}

bool
GzipFileStreamBuffer::seek(size_t position)
{
  ERRNO(gzseek(gzfile, position, SEEK_SET));
  marker = position;
  return true;
}

bool
MemoryStreamBuffer::seek(size_t position)
{
  if (position > data.size()) {
    return false;
  }
  marker = position;
  return true;
}

size_t
FileStreamBuffer::tell() const
{
  return ftell(file);
}

size_t
GzipFileStreamBuffer::tell() const
{
  return gztell(gzfile);
}

size_t
MemoryStreamBuffer::tell() const
{
  return marker;
}

void
FileStreamBuffer::backward(size_t count)
{
  // Get the current position
  long currentPos = ftell(file);

  if (currentPos == -1) {
    return; // Error: Failed to get current position
  }

  // Calculate the new position
  long newPos = currentPos - static_cast<long>(count);

  // Ensure the new position is not before the beginning of the file
  if (newPos < 0) {
    newPos = 0; // Clamp to the start of the file
  }

  ERRNO(fseek(file, newPos, SEEK_SET));
}

void
GzipFileStreamBuffer::backward(size_t count)
{
  // Get the current position
  long currentPos = gztell(gzfile);

  if (currentPos == -1) {
    return; // Error: Failed to get current position
  }

  // Calculate the new position
  long newPos = currentPos - static_cast<long>(count);

  // Ensure the new position is not before the beginning of the file
  if (newPos < 0) {
    newPos = 0; // Clamp to the start of the file
  }

  ERRNO(gzseek(gzfile, newPos, SEEK_SET));
}

void
MemoryStreamBuffer::backward(size_t count)
{
  marker -= count;
}

void
FileStreamBuffer::forward(size_t count)
{
  // Get the current position
  long currentPos = ftell(file);

  if (currentPos == -1) {
    return; // Error: Failed to get current position
  }

  // Calculate the new position
  long newPos = currentPos + static_cast<long>(count);

  // FIXME: clamp to 0 and file size? or fail?
  ERRNO(fseek(file, newPos, SEEK_SET));
}

void
GzipFileStreamBuffer::forward(size_t count)
{
  // Get the current position
  long currentPos = gztell(gzfile);

  if (currentPos == -1) {
    return; // Error: Failed to get current position
  }

  // Calculate the new position
  long newPos = currentPos + static_cast<long>(count);

  // FIXME: clamp to 0 and file size? or fail?
  ERRNO(gzseek(gzfile, newPos, SEEK_SET));
}

void
MemoryStreamBuffer::forward(size_t count)
{
  marker += count;
}

int
FileStreamBuffer::readInt()
{
  int temp;

  ERRNO(fread(&temp, sizeof(int), 1, file));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  marker += sizeof(int); // Do we track?  FILE has it, right?
  return temp;
}

short
FileStreamBuffer::readShort()
{
  short temp;

  ERRNO(fread(&temp, sizeof(short), 1, file));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  marker += sizeof(short); // Do we track?  FILE has it, right?
  return temp;
}

float
FileStreamBuffer::readFloat()
{
  float temp;

  ERRNO(fread(&temp, sizeof(short), 1, file));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

long
FileStreamBuffer::bytesRemaining()
{
  // Get the current position in the file
  long currentPos = ftell(file);

  if (currentPos == -1) {
    return -1; // Error: Failed to get current position
  }

  // Seek to the end of the file
  if (fseek(file, 0, SEEK_END) != 0) {
    return -1; // Error: Failed to seek to the end
  }

  // Get the total file size
  long fileSize = ftell(file);

  if (fileSize == -1) {
    return -1; // Error: Failed to get file size
  }

  // Restore the original position
  if (fseek(file, currentPos, SEEK_SET) != 0) {
    return -1; // Error: Failed to restore the position
  }

  // Calculate the number of bytes remaining
  return fileSize - currentPos;
}

int
GzipFileStreamBuffer::readInt()
{
  int temp;

  ERRNO(gzread(gzfile, &temp, sizeof(int)));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

short
GzipFileStreamBuffer::readShort()
{
  short temp;

  ERRNO(gzread(gzfile, &temp, sizeof(short)));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

float
GzipFileStreamBuffer::readFloat()
{
  float temp;

  ERRNO(gzread(gzfile, &temp, sizeof(float)));
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

int
MemoryStreamBuffer::readInt()
{
  // ERRNO
  if (marker + sizeof(int) > data.size()) {
    throw std::out_of_range("Not enough data in memory buffer to read int");
  }
  int temp;

  std::memcpy(&temp, &data[marker], sizeof(int));
  marker += sizeof(int);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

short
MemoryStreamBuffer::readShort()
{
  // ERRNO
  if (marker + sizeof(short) > data.size()) {
    throw std::out_of_range("Not enough data in memory buffer to read short");
  }
  short temp;

  std::memcpy(&temp, &data[marker], sizeof(short));
  marker += sizeof(short);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));

  return temp;
}

float
MemoryStreamBuffer::readFloat()
{
  // ERRNO
  if (marker + sizeof(float) > data.size()) {
    throw std::out_of_range("Not enough data in memory buffer to read float");
  }
  float temp;

  std::memcpy(&temp, &data[marker], sizeof(float));
  marker += sizeof(float);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));

  return temp;
}

char
FileStreamBuffer::readChar()
{
  // Wait, does ERRNO work here.  I think fread returns number of chars read?
  // FIXME: Harder everything
  char ch;

  fread(&ch, sizeof(char), 1, file);
  return ch;
}

std::string
FileStreamBuffer::readString(size_t length)
{
  std::string name(length, '\0'); // Allocate exact size
  size_t bytesRead = fread(&name[0], 1, length, file);

  if (bytesRead == 0) {
    return { }; // Return empty string on EOF or error
  }

  // Find first null terminator in read portion
  auto pos = std::find(name.begin(), name.begin() + bytesRead, '\0');

  // Trim string to before null terminator if found
  name.resize(std::distance(name.begin(), pos));
  return name;
}

char
GzipFileStreamBuffer::readChar()
{
  // Wait, does ERRNO work here.  I think fread returns number of chars read?
  // FIXME: Harder everything
  char ch;

  gzread(gzfile, &ch, sizeof(char));
  return ch;
}

std::string
GzipFileStreamBuffer::readString(size_t length)
{
  // FIXME: File and GZIPFile are different techniques
  std::vector<unsigned char> v;

  v.resize(length);
  const size_t bytes = length * sizeof(unsigned char);

  ERRNO(gzread(gzfile, &v[0], bytes));
  std::string name;

  // Use zero to finish std::string
  for (size_t i = 0; i < bytes; i++) {
    if (v[i] == 0) { break; }
    name += v[i];
  }

  return name;
}

char
MemoryStreamBuffer::readChar()
{
  // Check if there is enough data left to read
  if (marker + 1 > data.size()) {
    throw std::out_of_range("Not enough data in memory buffer to read char");
  }
  char ch = data[marker++];

  return ch;
}

std::string
MemoryStreamBuffer::readString(size_t length)
{
  // Check if there is enough data left to read
  if (marker + length > data.size()) {
    throw std::out_of_range("Not enough data in memory buffer to read string");
  }

  // Read `length` bytes from the memory buffer
  std::string result(length + 1, '\0'); // Create a string with space for `length` bytes + null terminator

  std::memcpy(&result[0], &data[marker], length);

  // Advance the marker
  marker += length;

  // Find the first occurrence of '\0' within the read portion
  auto pos = std::find(result.begin(), result.begin() + length, '\0');

  // Resize the string to remove any trailing zero bytes
  if (pos != result.end()) {
    result.resize(std::distance(result.begin(), pos));
  } else {
    result.resize(length); // Keep the full length if no null byte is found
  }

  return result;
}

void
BinaryIO::write_string16(const std::string& s, FILE * fp)
{
  size_t lenlong = s.size();

  if (lenlong > 65535) { lenlong = 65535; }
  unsigned short len = (unsigned short) (lenlong);

  fwrite(&len, sizeof(len), 1, fp);
  fwrite(s.c_str(), sizeof(char), len, fp);
}

void
BinaryIO::write_string8(const std::string& s, FILE * fp)
{
  size_t lenlong = s.size();

  if (lenlong > 255) { lenlong = 255; }
  unsigned char len = (unsigned char) (lenlong);

  fwrite(&len, sizeof(len), 1, fp);
  fwrite(s.c_str(), sizeof(char), len, fp);
}

void
BinaryIO::read_string16(std::string& s, FILE * fp, size_t maxlen)
{
  unsigned short len;

  fread(&len, sizeof(len), 1, fp);

  if (len > maxlen) { return; }

  rawReadString(s, len, fp);
}

void
BinaryIO::read_string8(std::string& s, FILE * fp)
{
  unsigned char len;

  fread(&len, sizeof(len), 1, fp);
  rawReadString(s, len, fp);
}

void
BinaryIO::writeScaledInt(gzFile fp, float w, float scale)
{
  int temp = w * scale;

  ON_BIG_ENDIAN(OS::byteswap(temp));
  ERRNO(gzwrite(fp, &temp, sizeof(int)));
}

int
BinaryIO::readInt(FILE * fp)
{
  int temp;

  ERRNO(fread(&temp, sizeof(int), 1, fp));
  ON_BIG_ENDIAN(OS::byteswap(temp));
  return temp;
}

int
BinaryIO::readShort(FILE * fp)
{
  short temp;

  ERRNO(fread(&temp, sizeof(short), 1, fp));
  ON_BIG_ENDIAN(OS::byteswap(temp));
  return temp;
}

void
BinaryIO::writeInt(gzFile fp, int w)
{
  int temp = w;

  ON_BIG_ENDIAN(OS::byteswap(temp));
  ERRNO(gzwrite(fp, &temp, sizeof(int)));
}

void
BinaryIO::writeFloat(gzFile fp, float w)
{
  float temp = w;

  ON_BIG_ENDIAN(OS::byteswap(temp));
  ERRNO(gzwrite(fp, &temp, sizeof(float)));
}

#if 0
std::string
BinaryIO::readChar(FILE * fp, size_t length)
{
  // Create a string with the exact size (uninitialized storage)
  std::string name(length + 1, '\0'); // Initializes string with `length` zero bytes

  // Read into the string's underlying buffer
  const size_t bytesRead = fread(&name[0], 1, length, fp);

  // Find the first occurrence of '\0'
  auto pos = std::find_first_of(name.begin(), name.begin() + bytesRead, "\0", "\0" + 1);

  // Resize the string to remove trailing zero bytes if found
  if (pos != name.end()) {
    name.resize(std::distance(name.begin(), pos)); // Shrink to the first zero byte
  }

  return name;
}

#endif // if 0

void
BinaryIO::writeChar(gzFile fp, std::string c, size_t length)
{
  if (length < 1) { return; }
  std::vector<unsigned char> v;

  v.resize(length);

  const size_t max = std::min(c.length(), length);

  // Write up to std::string chars
  for (size_t i = 0; i < max; i++) {
    v[i] = c[i];
  }
  // Fill with zeros
  for (size_t i = max; i < length; i++) {
    v[i] = 0;
  }

  const size_t bytes = length * sizeof(unsigned char);

  ERRNO(gzwrite(fp, &v[0], bytes));
}

Time
StreamBuffer::readTime(int year)
{
  // Time
  // We preread year in some cases during the guessing
  // of datatype since mrms binary doesn't have a type field
  // So we sometimes already have read the year
  if (year == -99) {
    year = readInt();
  }
  const int month = readInt();
  const int day   = readInt();
  const int hour  = readInt();
  const int min   = readInt();
  const int sec   = readInt();

  Time dataTime(year, month, day, hour, min, sec, 0.0);

  return dataTime;
}

void
BinaryIO::writeTime(gzFile fp, const Time& t)
{
  // Time
  BinaryIO::writeInt(fp, t.getYear());
  BinaryIO::writeInt(fp, t.getMonth());
  BinaryIO::writeInt(fp, t.getDay());
  BinaryIO::writeInt(fp, t.getHour());
  BinaryIO::writeInt(fp, t.getMinute());
  BinaryIO::writeInt(fp, t.getSecond());
}
