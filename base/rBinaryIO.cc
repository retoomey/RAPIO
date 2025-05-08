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

void
FileStreamBuffer::writeInt(int i)
{
  int temp = i;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  ERRNO(fwrite(&temp, sizeof(int), 1, file));
  marker += sizeof(int);
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

void
FileStreamBuffer::writeFloat(float f)
{
  float temp = f;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  ERRNO(fwrite(&temp, sizeof(float), 1, file));
  marker += sizeof(float);
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

void
GzipFileStreamBuffer::writeInt(int i)
{
  int temp = i;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  ERRNO(gzwrite(gzfile, &temp, sizeof(int)));
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

void
GzipFileStreamBuffer::writeFloat(float f)
{
  float temp = f;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  ERRNO(gzwrite(gzfile, &temp, sizeof(float)));
}

int
MemoryStreamBuffer::readInt()
{
  ensureAvailable(sizeof(int));
  int temp;

  std::memcpy(&temp, &data[marker], sizeof(int));
  marker += sizeof(int);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  return temp;
}

void
MemoryStreamBuffer::writeInt(int i)
{
  ensureAvailable(sizeof(int));

  int temp = i;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  std::memcpy(&data[marker], &temp, sizeof(int));
  marker += sizeof(int);
}

short
MemoryStreamBuffer::readShort()
{
  ensureAvailable(sizeof(short));
  short temp;

  std::memcpy(&temp, &data[marker], sizeof(short));
  marker += sizeof(short);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));

  return temp;
}

float
MemoryStreamBuffer::readFloat()
{
  ensureAvailable(sizeof(float));
  float temp;

  std::memcpy(&temp, &data[marker], sizeof(float));
  marker += sizeof(float);
  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));

  return temp;
}

void
MemoryStreamBuffer::writeFloat(float f)
{
  ensureAvailable(sizeof(float));

  float temp = f;

  ON_ENDIAN_DIFF(myDataBigEndian, OS::byteswap(temp));
  std::memcpy(&data[marker], &temp, sizeof(float));
  marker += sizeof(float);
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

void
FileStreamBuffer::writeString(const std::string& c, size_t upto)
{
  int toWrite = std::min<int>(c.size(), upto);

  if (fwrite(c.data(), 1, toWrite, file) != static_cast<size_t>(toWrite)) {
    throw std::runtime_error("Failed to write string to file");
  }

  marker += toWrite;

  // Pad with nulls if string is shorter than `upto`
  if (toWrite < upto) {
    int pad = upto - toWrite;
    static const char zeros[16] = { }; // small zero buffer

    while (pad > 0) {
      int chunk = std::min(pad, static_cast<int>(sizeof(zeros)));
      if (fwrite(zeros, 1, chunk, file) != static_cast<size_t>(chunk)) {
        throw std::runtime_error("Failed to write null padding to file");
      }
      marker += chunk;
      pad    -= chunk;
    }
  }
}

void
FileStreamBuffer::readVector(void * data, size_t size)
{
  size_t bytesRead = fread(data, 1, size, file);

  if (bytesRead != size) {
    throw std::runtime_error("Failed to read vector from file stream");
  }
}

void
FileStreamBuffer::writeVector(const void * data, size_t size)
{
  if (fwrite(data, 1, size, file) != size) {
    throw std::runtime_error("Failed to write vector to file stream");
  }
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
  std::string name(length, '\0'); // Pre-allocate buffer
  int bytesRead = gzread(gzfile, &name[0], length);

  if (bytesRead <= 0) {
    return { }; // Error or EOF
  }

  // Trim at first null terminator
  auto pos = std::find(name.begin(), name.begin() + bytesRead, '\0');

  name.resize(std::distance(name.begin(), pos));
  return name;
}

void
GzipFileStreamBuffer::writeString(const std::string& c, size_t upto)
{
  size_t toWrite = std::min(c.size(), upto);

  if (gzwrite(gzfile, c.data(), toWrite) != static_cast<int>(toWrite)) {
    throw std::runtime_error("Failed to write string to gzip stream");
  }

  marker += toWrite;

  // Optionally pad with nulls if needed
  if (toWrite < upto) {
    size_t pad = upto - toWrite;
    static const char zeros[16] = { };

    while (pad > 0) {
      size_t chunk = std::min(pad, sizeof(zeros));
      if (gzwrite(gzfile, zeros, chunk) != static_cast<int>(chunk)) {
        throw std::runtime_error("Failed to write null padding to gzip stream");
      }
      marker += chunk;
      pad    -= chunk;
    }
  }
}

void
GzipFileStreamBuffer::readVector(void * data, size_t size)
{
  int bytesRead = gzread(gzfile, data, static_cast<unsigned int>(size));

  if (bytesRead != static_cast<int>(size)) {
    throw std::runtime_error("Failed to read vector from gzip stream");
  }

  marker += bytesRead;
}

void
GzipFileStreamBuffer::writeVector(const void * data, size_t size)
{
  if (gzwrite(gzfile, data, size) != static_cast<int>(size)) {
    throw std::runtime_error("Failed to write vector to gzip stream");
  }
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
StreamBuffer::writeTime(const Time& t)
{
  // Time
  writeInt(t.getYear());
  writeInt(t.getMonth());
  writeInt(t.getDay());
  writeInt(t.getHour());
  writeInt(t.getMinute());
  writeInt(t.getSecond());
}
