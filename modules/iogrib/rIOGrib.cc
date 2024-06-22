#include "rIOGrib.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rConfig.h"
#include "rConfigIODataType.h"

#include "rGribDataTypeImp.h"
#include "rGribMessageImp.h"
#include "rGribDatabase.h"
#include "rGribAction.h"

#include <fstream>

using namespace rapio;

// NCEPLIBS changed it from GC_VERSION to G2C_VERSION around version 1.8
#ifdef G2C_VERSION
# define GRIB2_VERSION_STRING G2C_VERSION
#else
# ifdef GC_VERSION
#  define GRIB2_VERSION_STRING GC_VERSION
# else
#  define GRIB2_VERSION_STRING "Unknown"
# endif
#endif

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOGrib();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOGrib::getGrib2Error(g2int ierr)
{
  std::string err = "No Error";

  switch (ierr) {
      case 0:
        break;
      case 1: err = "Beginning characters GRIB not found";
        break;
      case 2: err = "Grib message is not Edition 2";
        break;
      case 3: err = "Could not find Section 1, where expected";
        break;
      case 4: err = "End string '7777' found, but not where expected";
        break;
      case 5: err = "End string '7777' not found at end of message";
        break;
      case 6: err = "Invalid section number found";
        break;
      default:
        err = "Unknown grib2 error: " + std::to_string(ierr);
        break;
  }
  return err;
}

std::shared_ptr<Array<float, 2> >
IOGrib::get2DData(std::shared_ptr<GribMessage>& m, size_t fieldNumber)
{
  auto gfld = m->getField(fieldNumber);

  if (gfld == nullptr) {
    return nullptr;
  }
  return gfld->getFloat2D();
} // IOGrib::get2DData

std::shared_ptr<Array<float, 3> >
IOGrib::get3DData(std::vector<std::shared_ptr<GribMessage> >& mv, const std::vector<size_t>& fieldN,
  const std::vector<std::string>& levels)
{
  std::shared_ptr<Array<float, 3> > newOne = nullptr;
  const size_t numZ = mv.size();

  // Iterate through found fields, if they are all grids of same size,
  // return a 3D array of them.
  for (size_t i = 0; i < numZ; i++) {
    auto field = mv[i]->getField(fieldN[i]);
    if (field == nullptr) {
      LogSevere("Couldn't unpack level " << i << " of request 3D field.\n");
      return nullptr; // Humm..could just have empty layer?
    } else {
      LogInfo("Unpack Level '" << levels[i] << "' (" << i << ") successfully.\n");
      // Fill this level...
      newOne = field->getFloat3D(newOne, i, numZ);
    }
  }
  return newOne;
} // IOGrib::get3DData

namespace {
// FIXME: Maybe these should be static to iogrib?

/** My version of gbit which appears much smaller and faster */
inline g2int
rbit(unsigned char * bu, size_t start, size_t length)
{
  g2int out;
  size_t s = start;

  for (size_t i = 0; i < length; i++) {
    out = (out << 8) | bu[s++];
  }
  return out;
}

/** Read X bytes from a file stream. Could be stuck in file utils */
inline bool
// readAt(std::fstream& file, std::streampos at, std::vector<unsigned char>& buffer, std::streamsize count)
readAt(std::fstream& file, std::streampos at, unsigned char * buffer, std::streamsize count)
{
  file.seekg(at, std::ios::beg);
  // Note: we use unsigned char everywhere else because of bit logic
  file.read(reinterpret_cast<char *>(buffer), count);
  if (file.fail()) {
    LogSevere("Error reading: eof? " << file.eof() << " bad? " << file.bad() << "\n");
    return false;
  }
  return true;
}

/** Get the parts of a grib2 message header we care about */
inline bool
getGrib2Header(unsigned char * b, g2int& version, g2int& length)
{
  // Check start for GRIB '1196575042' magic number
  version = -1;
  length  = 0;
  if ((b[0] == 'G') && (b[1] == 'R') && (b[2] == 'I') &&
    (b[3] == 'B') )
  {
    // Get version number and length
    version = b[7];
    if (version == 2) {
      length = rbit(&b[12], 0, 4);
    } else if (version == 1) {
      length = rbit(&b[4], 0, 3); // FIXME: test this with a grib1 file?
    } else {
      return false;
    }
    return true;
  }
  return false;
}

/** Do we have the end of GRIB2 message? */
inline bool
haveGrib2Footer(unsigned char * b)
{
  // '926365495' magic number or '7777'
  return (b[0] == '7' && b[1] == '7' && b[2] == '7' && b[3] == '7');
}

/** Process a single grib2 message */
inline bool
processGribMessage(GribAction& a, size_t fileOffset, size_t messageCount, std::shared_ptr<GribMessage> mp)
{
  // If we can read a GRIB2 message...
  auto& m = *mp;

  if (m.readG2Info(messageCount, fileOffset)) {
    size_t numfields = m.getNumberFields();

    // ...then handle each field of the message
    for (size_t f = 1; f <= numfields; ++f) {
      // Allowing a GribAction to process it and gather information
      const bool keepGoing = a.action(mp, f);
      if (!keepGoing) {
        return false;
      } // stop if action asks
    }
  }
  return true; // continue to next message since strategy didn't say stop
} // processGribMessage
}

bool
IOGrib::scanGribDataFILE(const URL& url, GribAction * ap, GribDataTypeImp* o)
{
  // For now since this can be a long operation, notify whenever we call it
  LogInfo("Scanning grib2 using per field mode (RAM per message)...\n");

  // Make sure we always have an action/strategy defined to avoid checks in loop
  // It doesn't make sense to scan without doing 'something' even if just printing
  GribAction noAction;
  GribAction& a = (ap == nullptr) ? noAction : *ap;

  // Get information on the file
  // FIXME: check local?
  std::fstream file(url.toString(), std::ios::in | std::ios::binary);

  if (!file) {
    LogSevere("Unable to read local file at " << url << "\n");
    return false;
  }

  // Get the file size
  file.seekg(0, std::ios::end);
  std::streampos aSize = file.tellg();

  file.seekg(0, std::ios::beg);

  // Mini buffer for info reads
  std::vector<unsigned char> mini(16);

  // Full message buffer
  std::vector<unsigned char> buffer;

  // While we have grib2 messages...
  size_t messageCount = 0;
  size_t k      = 0;
  g2int lengrib = 0;
  g2int version = -1;

  while (k < aSize) {
    // Read 16 bytes to check GRIB, version number and message length
    if (!readAt(file, k, mini.data(), 16)) { return false; }

    // Now if we have a GRIB header...
    if (getGrib2Header(&mini[0], version, lengrib)) {
      // Go back 4 bytes from length to read the end length
      // So we're skipping over message to the end
      const size_t at = k + lengrib - 4;
      if (at + 3 < aSize) {
        // Read footer marker
        if (!readAt(file, at, mini.data(), 4)) { return false; }

        if (haveGrib2Footer(&mini[0])) {
          // Get the message info
          messageCount++;

          // Actually read/store into the message since we don't have
          // a global buffer.  We have to store temp to at least read the
          // field info.
          // Actions might want to hold onto us for future use, so use shared_ptr
          // Might be faster to use objects and 'moves' but easy to mess that up
          auto m = std::make_shared<GribMessageImp>((size_t) lengrib, o->getValidityPtr());

          // Go ahead and read the entire record now
          if (!readAt(file, k, m->getBufferPtr(), lengrib)) { return false; }

          // And then process the field, sending to strategy, if
          // we're not to continue, return true and we're done scanning
          if (!processGribMessage(a, k, messageCount, m)) {
            return true;
          }
        } else { // footer missing.  Short file maybe
          LogSevere("GRIB footer 7777 missing.  Short data maybe?\n");
          break;
        }
      } else {
        LogSevere("GRIB buffer overflow. Short data maybe?\n");
        break;
      }
    } else {
      LogSevere("No GRIB data in buffer or unhandled GRIB version\n");
      break;
    }
    k += lengrib; // next message...
  }

  LogInfo("Total messages: " << messageCount << "\n");
  return true;
} // IOGrib::scanGribDataFILE

bool
IOGrib::scanGribData(std::vector<char>& b, GribAction * ap, GribDataTypeImp* o)
{
  // For now since this can be a long operation, notify whenever we call it
  LogInfo("Scanning grib2 using FULL buffer mode (RAM hogging)...\n");

  // Make sure we always have an action/strategy defined to avoid checks in loop
  // It doesn't make sense to scan without doing 'something' even if just printing
  GribAction noAction;
  GribAction& a = (ap == nullptr) ? noAction : *ap;

  // Get information on the buffer
  const size_t aSize = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);

  // Message information from g2_info
  g2int listsec0[3], listsec1[13], numfields, numlocal;

  // While we have grib2 messages...
  size_t messageCount = 0;
  size_t k      = 0;
  g2int lengrib = 0;
  g2int version;

  while (k < aSize) {
    // Now if we have a GRIB header...
    if (getGrib2Header(&bu[k], version, lengrib)) {
      // Go back 4 bytes from length to read the end length
      // So we're skipping over message to the end
      const size_t at = k + lengrib - 4;
      if (at + 3 < aSize) {
        if (haveGrib2Footer(&bu[at])) {
          // Get the message info
          messageCount++;

          auto m = std::make_shared<GribMessageImp>(bu + k, o->getValidityPtr());

          // And then process the field, sending to strategy, if
          // we're not to continue, return true and we're done scanning
          if (!processGribMessage(a, k, messageCount, m)) {
            return true;
          }
          // End of message match...
        } else { // footer missing.  Short file maybe
          LogSevere("GRIB footer 7777 missing.  Short data maybe?\n");
          break;
        }
      } else {
        LogSevere("GRIB buffer overflow. Short data maybe?\n");
        break;
      }
    } else {
      LogSevere("No GRIB data in buffer or unhandled GRIB version\n");
      break;
    }
    k += lengrib; // next message...
  }

  LogInfo("Total messages: " << messageCount << "\n");
  return true;
} // myseekgbbuf

std::string
IOGrib::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the grib/grib2 library to read grib data files.";
  return help;
}

void
IOGrib::initialize()
{
  LogInfo("GRIB module using grib library version: " << GRIB2_VERSION_STRING << "\n");
}

std::shared_ptr<DataType>
IOGrib::readGribDataType(const URL& url, int mode)
{
  // HACK IN MY GRIB.data thing for moment...
  // Could lazy read only on string matching...
  GribDatabase::readGribDatabase();

  std::vector<char> buf;

  // For mode 1, prefill in the entire buffer with the file.
  if (mode == 1) {
    IOURL::read(url, buf);

    if (buf.empty()) {
      LogSevere("Couldn't read data for grib2 at " << url << "\n");
      return nullptr;
    }
  }

  // Other modes just send onto the object to handle later
  std::shared_ptr<GribDataTypeImp> g = std::make_shared<GribDataTypeImp>(url, buf, mode);

  return g;
} // IOGrib::readGribDataType

std::shared_ptr<DataType>
IOGrib::createDataType(const std::string& params)
{
  // Hack get rapiosetting.xml for grib
  // FIXME: We probably should generalize higher up.  This is first time I've wanted
  // a param for reading vs writing. So design should change and move this
  // into IODataType
  std::string theMode = "0"; // default to reading by message
  int mode = 0;
  std::shared_ptr<PTreeNode> dfs = ConfigIODataType::getSettings("grib");

  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      auto test   = output.getAttr("bmode", (std::string) "0");
      if (test == "1") {
        mode = 1;
      } else {
        mode = 0;
      }
    }catch (const std::exception& e) {
      // It's ok, default to 0
    }
  }

  // virtual to static, we only handle file/url
  return (IOGrib::readGribDataType(URL(params), mode));
}

bool
IOGrib::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & params
)
{
  return false;
}

IOGrib::~IOGrib(){ }
