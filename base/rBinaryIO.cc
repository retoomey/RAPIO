#include "rBinaryIO.h"

#include <rError.h>

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
