#include "rBinaryIO.h"

#include <rError.h>
#include <rOS.h> // FIXME: temp

using namespace rapio;
using namespace std;

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

float
BinaryIO::readScaledInt(gzFile fp, float scale)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
  ON_BIG_ENDIAN(OS::byteswap(temp));
  return float(temp) / scale;
}

void
BinaryIO::writeScaledInt(gzFile fp, float w, float scale)
{
  int temp = w * scale;

  ON_BIG_ENDIAN(OS::byteswap(temp));
  ERRNO(gzwrite(fp, &temp, sizeof(int)));
}

int
BinaryIO::readInt(gzFile fp)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
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

int
BinaryIO::readFloat(gzFile fp)
{
  float temp;

  ERRNO(gzread(fp, &temp, sizeof(float)));
  ON_BIG_ENDIAN(OS::byteswap(temp));
  return temp;
}

void
BinaryIO::writeFloat(gzFile fp, float w)
{
  float temp = w;

  ON_BIG_ENDIAN(OS::byteswap(temp));
  ERRNO(gzwrite(fp, &temp, sizeof(float)));
}

std::string
BinaryIO::readChar(gzFile fp, size_t length)
{
  std::vector<unsigned char> v;

  v.resize(length);
  const size_t bytes = length * sizeof(unsigned char);

  ERRNO(gzread(fp, &v[0], bytes));
  std::string name;

  // Use zero to finish std::string
  for (size_t i = 0; i < bytes; i++) {
    if (v[i] == 0) { break; }
    name += v[i];
  }

  return name;
}

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
BinaryIO::readTime(gzFile fp, int year)
{
  // Time
  // We preread year in some cases during the guessing
  // of datatype since mrms binary doesn't have a type field
  // So we sometimes already have read the year
  if (year == -99) {
    year = BinaryIO::readInt(fp);
  }
  const int month = BinaryIO::readInt(fp);
  const int day   = BinaryIO::readInt(fp);
  const int hour  = BinaryIO::readInt(fp);
  const int min   = BinaryIO::readInt(fp);
  const int sec   = BinaryIO::readInt(fp);

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
