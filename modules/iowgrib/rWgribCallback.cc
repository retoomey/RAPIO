#include "rWgribCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"

#include <sstream>

using namespace rapio;

namespace {
uint64_t
GB2_Int8(const unsigned char * p)
{
  return ((uint64_t) p[0] << 56)
         | ((uint64_t) p[1] << 48)
         | ((uint64_t) p[2] << 40)
         | ((uint64_t) p[3] << 32)
         | ((uint64_t) p[4] << 24)
         | ((uint64_t) p[5] << 16)
         | ((uint64_t) p[6] << 8)
         | ((uint64_t) p[7]);
}

uint16_t
GB2_Int2(const unsigned char * p)
{
  return ((uint16_t) p[0] << 8) | p[1];
}
}

void
WgribCallback::handleSetFieldInfo(unsigned char ** sec, int msg_no, int submsg, long int pos)
{
  myMessageNumber = msg_no; // Message number
  myFieldNumber   = submsg; // Field number
  myFilePosition  = pos;

  if (!sec || !sec[0] || !sec[1]) { return; }

  // Section 0:  Discipline (1 byte), Edition (1 byte), Total Length (8 bytes)
  mySection0[0] = sec[0][6];            // Discipline
  mySection0[1] = sec[0][7];            // GRIB Edition
  mySection0[2] = GB2_Int8(sec[0] + 8); // Message Length (8-byte int)

  // Section 1: Center, Subcenter, Master Table, RefTime, etc.
  mySection1[0]  = GB2_Int2(sec[1] + 5);  // Originating center
  mySection1[1]  = GB2_Int2(sec[1] + 7);  // Subcenter
  mySection1[2]  = sec[1][9];             // Master Table version
  mySection1[3]  = sec[1][10];            // Local Table version
  mySection1[4]  = sec[1][11];            // Significance of RT
  mySection1[5]  = GB2_Int2(sec[1] + 12); // Year
  mySection1[6]  = sec[1][14];            // Month
  mySection1[7]  = sec[1][15];            // Day
  mySection1[8]  = sec[1][16];            // Hour
  mySection1[9]  = sec[1][17];            // Minute
  mySection1[10] = sec[1][18];            // Second
  mySection1[11] = sec[1][19];            // Production status
  mySection1[12] = sec[1][20];            // Data type (e.g. analysis, forecast)

  Time theTime = Time(mySection1[5], // year
      mySection1[6],                 // month
      mySection1[7],                 // day
      mySection1[8],                 // hour
      mySection1[9],                 // minute
      mySection1[10],                // second
      0.0                            // fractional
  );

  myTime = theTime;
} // WgribCallback::handleSetFieldInfo

std::vector<std::string>
WgribCallback::execute(bool print, bool capture)
{
  RAPIOCallbackCPP catalog(this);

  // Create the args for wgrib2
  std::vector<std::string> args;

  args.push_back("wgrib2");
  args.push_back(myFilename);

  #if 0
  if (!myMatch.empty()) {
    args.push_back("-match");
    args.push_back(myMatch);
  }
  #endif

  // Allow subclasses to add extra args
  addExtraArgs(args);

  // Convert pointer to integer for callback
  args.push_back("-rapio");
  std::ostringstream oss;

  oss << reinterpret_cast<uintptr_t>(&catalog);
  std::string pointer = oss.str();

  args.push_back(pointer);

  // wgrib2 wants a c style array, so convert,
  // the vector will be in scope
  std::vector<const char *> argv;

  for (const auto& arg : args) {
    argv.push_back(arg.c_str());
  }
  int argc = argv.size();

  // Print the arguments to command line
  std::ostringstream param;

  bool nextQuoted  = false;
  bool nextIgnored = false;

  for (size_t i = 0; i < argc; ++i) {
    std::string a = argv[i];

    if (a == "-rapio") { nextIgnored = true; continue; }
    if (nextIgnored) { nextIgnored = false; continue; }

    // quote match for shell. Let's us copy/paste for debugging
    if (nextQuoted) {
      param << "\"" << a << "\"";
      nextQuoted = false;
    } else {
      param << a;
    }
    if (a == "-match") { nextQuoted = true; }
    if (i < argc - 1) { param << " "; }
  }
  fLogInfo("Calling: {}", param.str());

  // Execute and capture output
  std::vector<std::string> result =
    IOWgrib::capture_vstdout_of_wgrib2(capture, argv.size(), argv.data());

  if (capture && print) {
    for (size_t i = 0; i < result.size(); ++i) {
      fLogInfo("[wgrib2] {}", result[i]);
    }
  }
  return result;
} // WgribCallback::execute
