#include "rIOGrib.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rConfig.h"

#include "rGribDataTypeImp.h"
#include "rGribDatabase.h"
#include "rGribAction.h"

using namespace rapio;

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

extern "C" {
#include <grib2.h>
}

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
IOGrib::get2DData(std::vector<char>& b, size_t at, size_t fieldNumber, size_t& x, size_t& y)
{
  // size_t aSize       = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);
  // Reread found data, but slower now, unpack it
  gribfield * gfld = 0; // output
  int ierr         = g2_getfld(&bu[at], fieldNumber, 1, 1, &gfld);

  if (ierr == 0) {
    LogInfo("Grib2 field unpack successful\n");
  }
  const g2int * gds = gfld->igdtmpl;
  int tX      = gds[7]; // 31-34 Nx -- Number of points along the x-axis
  int tY      = gds[8]; // 35-38 Ny -- Number of points along te y-axis
  size_t numX = (tX < 0) ? 0 : (size_t) (tX);
  size_t numY = (tY < 0) ? 0 : (size_t) (tY);

  LogInfo("Grib2 2D field size " << numX << " * " << numY << "\n");
  x = numX;
  y = numY;
  g2float * g2grid = gfld->fld;

  auto newOne = Arrays::CreateFloat2D(numX, numY);
  auto& data  = newOne->ref();

  for (size_t xf = 0; xf < numX; ++xf) {
    for (size_t yf = 0; yf < numY; ++yf) {
      data[xf][yf] = (float) (g2grid[yf * numX + xf]);
    }
  }

  g2_free(gfld);

  return newOne;
} // IOGrib::get2DData

std::shared_ptr<Array<float, 3> >
IOGrib::get3DData(std::vector<char>& b, const std::string& key, const std::vector<std::string>& levelsStringVec,
  size_t& x, size_t&y, size_t& z, float missing)
{
  size_t zVecSize = levelsStringVec.size();
  size_t nz       = (zVecSize == 0) ? 40 : (size_t) zVecSize;

  size_t nx   = (x == 0) ? 1 : x;
  size_t ny   = (y == 0) ? 1 : y;
  auto newOne = Arrays::CreateFloat3D(nx, ny, nz);
  auto &data  = newOne->ref();

  // std::shared_ptr<RAPIO_3DF> newOne = std::make_shared<RAPIO_3DF>(RAPIO_DIM3(nx, ny, nz));
  // auto &data = *newOne;

  unsigned char * bu = (unsigned char *) (&b[0]);

  for (size_t k = 0; k < nz; ++k) {
    GribMatcher test(key, levelsStringVec[k]);
    IOGrib::scanGribData(b, &test);

    size_t at, fieldNumber;
    if (test.getMatch(at, fieldNumber)) {
      gribfield * gfld = 0; // output

      int ierr = g2_getfld(&bu[at], fieldNumber, 1, 1, &gfld);

      if (ierr == 0) {
        LogInfo("Grib2 field unpack successful\n");
      }
      const g2int * gds = gfld->igdtmpl;
      int tX      = gds[7]; // 31-34 Nx -- Number of points along the x-axis
      int tY      = gds[8]; // 35-38 Ny -- Number of points along te y-axis
      size_t numX = (tX < 0) ? 0 : (size_t) (tX);
      size_t numY = (tY < 0) ? 0 : (size_t) (tY);

      x = numX;
      y = numY;

      LogInfo("Grib2 3D field size " << numX << " * " << numY << " * " << nz << "\n");
      if ((nx != x) || (ny != y)) {
        LogInfo("Warning! sizes do not match" << nx << " != " << x << " and/or " << ny << " != " << y << "\n");
      }

      g2float * g2grid = gfld->fld;

      for (size_t xf = 0; xf < numX; ++xf) {
        for (size_t yf = 0; yf < numY; ++yf) {
          data[xf][yf][k] = (float) (g2grid[yf * numX + xf]);
        }
      }

      g2_free(gfld);
    } // test.getMatch
    else {
      LogSevere("No data for " << key << " at " << levelsStringVec[k] << "\n");

      // FIXME: The desired situation has to be confirmed - exit, which will help ensure the mistake is corrected, or a log
      // exit(1);

      for (size_t xf = 0; xf < nx; ++xf) {
        for (size_t yf = 0; yf < ny; ++yf) {
          data[xf][yf][k] = missing;
        }
      }
    }
  }
  // g2_free(gfld);
  z = nz;

  return newOne; // nullptr;
} // IOGrib::get3DData

namespace {
/** Create my own copy of gbits and gbit.  It looks like the old implementation overflows
 * on large files since it counts number of bits so large files say 1 GB can overflow
 * the g2int size which is an int.
 * g2clib isn't that big and it appears not updated, so we may branch and do some work here.*/
void
_gbits(unsigned char * in, g2int * iout, unsigned long long iskip, g2int nbyte, g2int nskip,
  g2int n)

/*          Get bits - unpack bits:  Extract arbitrary size values from a
 * /          packed bit string, right justifying each value in the unpacked
 * /          iout array.
 * /           *in    = pointer to character array input
 * /           *iout  = pointer to unpacked array output
 * /            iskip = initial number of bits to skip
 * /            nbyte = number of bits to take
 * /            nskip = additional number of bits to skip on each iteration
 * /            n     = number of iterations
 * / v1.1
 */
{
  g2int i, tbit, bitcnt, ibit, itmp;
  unsigned long long nbit, index;
  static g2int ones[] = { 1, 3, 7, 15, 31, 63, 127, 255 };

  //     nbit is the start position of the field in bits
  nbit = iskip;
  for (i = 0; i < n; i++) {
    bitcnt = nbyte;
    index  = nbit / 8;
    ibit   = nbit % 8;
    nbit   = nbit + nbyte + nskip;

    //        first byte
    tbit = ( bitcnt < (8 - ibit) ) ? bitcnt : 8 - ibit; // find min

    itmp = (int) *(in + index) & ones[7 - ibit];

    if (tbit != 8 - ibit) { itmp >>= (8 - ibit - tbit); }
    index++;
    bitcnt = bitcnt - tbit;

    //        now transfer whole bytes
    while (bitcnt >= 8) {
      itmp   = itmp << 8 | (int) *(in + index);
      bitcnt = bitcnt - 8;
      index++;
    }

    //        get data from last byte
    if (bitcnt > 0) {
      itmp = ( itmp << bitcnt ) | ( ((int) *(in + index) >> (8 - bitcnt)) & ones[bitcnt - 1] );
    }

    *(iout + i) = itmp;
  }
} // _gbits

/** gbit just passes to gbit2 */
void
_gbit(unsigned char * in, g2int * iout, unsigned long long iskip, g2int nbyte)
{
  _gbits(in, iout, iskip, nbyte, (g2int) 0, (g2int) 1);
}
}

bool
IOGrib::scanGribDataFILE(FILE * lugb, GribAction * a)
{
  LogSevere("Ok in the scan grib data file class\n");
  LogSevere("Humm this doesn't work, ALPHA play here\n");

  /*
   * @param lugb FILE pointer for the file to search. File must be
   * opened before this routine is called.
   * @param iseek The number of bytes in the file to skip before search.
   * @param mseek The maximum number of bytes to search at a time (must
   * be at least 16, but larger numbers like 4092 will result in better
   * perfomance).
   * @param lskip Pointer that gets the number of bytes to skip from the
   * beggining of the file to where the GRIB message starts.
   * @param lgrib Pointer that gets the number of bytes in message (set
   * to 0, if no message found).
   * //seekgb(FILE *lugb, g2int iseek, g2int mseek, g2int *lskip, g2int *lgrib)
   */
  // Ugghgh grib2 is all c and messy and it could be done in c++ so easy, but
  // then when they change the format the c++ will break.  So bleh.
  //
  // stuff passed in
  //    FILE *lugb;
  g2int iseek = 0;                 // Number of bytes to skip before search, eh?
  g2int mseek = 100 * 1024 * 1024; // 100 MB
  g2int buffer[2];
  g2int * lskip = &buffer[0];
  g2int * lgrib = &buffer[1];

  g2int k, k4, ipos, nread, lim, start, vers, lengrib;
  int end;
  unsigned char * cbuf;

  // LOG((3, "seekgb iseek %ld mseek %ld", iseek, mseek));

  *lgrib = 0;
  cbuf   = (unsigned char *) malloc(mseek);
  nread  = mseek;
  ipos   = iseek;

  /* Loop until grib message is found. */
  while (*lgrib == 0 && nread == mseek) {
    /* Read partial section. */
    fseek(lugb, ipos, SEEK_SET);
    nread = fread(cbuf, sizeof(unsigned char), mseek, lugb);
    lim   = nread - 8;

    LogSevere("OK read in " << mseek << " size \n");
    /* Look for 'grib...' in partial section. */
    for (k = 0; k < lim; k++) {
      /* Look at the first 4 bytes - should be 'GRIB'. */
      gbit(cbuf, &start, k * BYTE, 4 * BYTE);
      // LogSevere("..."<<cbuf[0]<<cbuf[1]<<cbuf[2]<<cbuf[3]<<"\n");

      /* Look at the 8th byte, it has the GRIB version. */
      gbit(cbuf, &vers, (k + 7) * BYTE, 1 * BYTE);

      /* If the message starts with 'GRIB', and is version 1 or
       * 2, then this is a GRIB message. */
      LogSevere("GRIB VERSION: " << (int) (cbuf[0]) << "\n");
      if ((start == 1196575042) && ((vers == 1) || (vers == 2))) {
        /* Find the length of the message. */
        if (vers == 1) {
          gbit(cbuf, &lengrib, (k + 4) * BYTE, 3 * BYTE);
        }
        if (vers == 2) {
          gbit(cbuf, &lengrib, (k + 12) * BYTE, 4 * BYTE);
        }
        // LOG((4, "lengrib %ld", lengrib));
        LogSevere("Length of message " << lengrib << "\n");

        /* Read the last 4 bytesof the message. */
        fseek(lugb, ipos + k + lengrib - 4, SEEK_SET);
        k4 = fread(&end, 4, 1, lugb);

        /* Look for '7777' at end of grib message. */
        if ((k4 == 1) && (end == 926365495) ) {
          /* GRIB message found. */
          *lskip = ipos + k;
          *lgrib = lengrib;
          // LOG((4, "found end of message lengrib %ld", lengrib));
          LogSevere("End of message " << lengrib << "\n");
          break;
        }
      }
    }
    ipos = ipos + lim;
  }

  free(cbuf);
  return true;
} // IOGrib::scanGribDataFILE

bool
IOGrib::scanGribData(std::vector<char>& b, GribAction * a)
{
  size_t aSize       = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);
  // LogSevere("GRIB2 incoming: buffer size " << aSize << "\n");

  // Seekgb is actually kinda simple, it finds a single grib2 message and its
  // length into a FILE*.  We want to use a buffer though..this gives us the
  // advantage of being able to use a URL/ram, etc.
  // seekdb.c has the c code for this from a FILE*.  The downside
  // to us doing this is that we need to check future versions of seekdb.c
  // for bug fixes/changes.
  size_t messageCount = 0;
  // size_t k = 0;
  unsigned long long k = 0;


  LogInfo("Scan grib2 called with grib library version: " << G2_VERSION << "\n");
  g2int lengrib = 0;

  while (k < aSize) {
    g2int start = 0;
    g2int vers  = 0;
    gbit(bu, &start, (k + 0) * 8, 4 * 8);
    gbit(bu, &vers, (k + 7) * 8, 1 * 8);

    if (( start == 1196575042) && (( vers == 1) || ( vers == 2) )) {
      //  LOOK FOR '7777' AT END OF GRIB MESSAGE
      if (vers == 1) {
        gbit(bu, &lengrib, (k + 4) * 8, 3 * 8);
      }
      if (vers == 2) {
        gbit(bu, &lengrib, (k + 12) * 8, 4 * 8);
      }
      //    ret=fseek(lugb,ipos+k+lengrib-4,SEEK_SET);
      const size_t at = k + lengrib - 4;
      if (at + 3 < aSize) {
        g2int k4 = 1;
        //    k4=fread(&end,4,1,lugb); ret fails overload
        int end = int((unsigned char) (b[at]) << 24
          | (unsigned char) (b[at + 1]) << 16
          | (unsigned char) (b[at + 2]) << 8
          | (unsigned char) (b[at + 3]));
        if (( k4 == 1) && ( end == 926365495) ) { // GRIB message found
          messageCount++;
          //   OUTPUT ARGUMENTS:
          //     listsec0 - pointer to an array containing information decoded from
          //                GRIB Indicator Section 0.
          //                Must be allocated with >= 3 elements.
          //                listsec0[0]=Discipline-GRIB Master Table Number
          //                            (see Code Table 0.0)
          //                listsec0[1]=GRIB Edition Number (currently 2)
          //                listsec0[2]=Length of GRIB message
          //     listsec1 - pointer to an array containing information read from GRIB
          //                Identification Section 1.
          //                Must be allocated with >= 13 elements.
          //                listsec1[0]=Id of orginating centre (Common Code Table C-1)
          //                listsec1[1]=Id of orginating sub-centre (local table)
          //                listsec1[2]=GRIB Master Tables Version Number (Code Table 1.0)
          //                listsec1[3]=GRIB Local Tables Version Number
          //                listsec1[4]=Significance of Reference Time (Code Table 1.1)
          //                listsec1[5]=Reference Time - Year (4 digits)
          //                listsec1[6]=Reference Time - Month
          //                listsec1[7]=Reference Time - Day
          //                listsec1[8]=Reference Time - Hour
          //                listsec1[9]=Reference Time - Minute
          //                listsec1[10]=Reference Time - Second
          //                listsec1[11]=Production status of data (Code Table 1.2)
          //                listsec1[12]=Type of processed data (Code Table 1.3)
          //     numfields- The number of gridded fields found in the GRIB message.
          //                That is, the number of occurences of Sections 4 - 7.
          //     numlocal - The number of Local Use Sections ( Section 2 ) found in
          //                the GRIB message.
          //

          g2int listsec0[3], listsec1[13], numlocal;
          g2int myNumFields = 0;
          int ierr = g2_info(&bu[k], listsec0, listsec1, &myNumFields, &numlocal);
          if (ierr > 0) {
            LogSevere(getGrib2Error(ierr));
          } else {
            if (a != nullptr) {
              a->setG2Info(messageCount, k, listsec0, listsec1, numlocal);
            }
            // Can check center first before info right?
            // g2int cntr = listsec1[0]; // Id of orginating centre
            // g2int mtab = listsec1[2]; // =GRIB Master Tables Version Number (Code Table 1.0)

            const size_t fieldNum = (myNumFields < 0) ? 0 : (size_t) (myNumFields);
            for (size_t f = 1; f <= fieldNum; ++f) {
              gribfield * gfld = 0; // output
              g2int unpack     = 0; // 0 do not unpack
              g2int expand     = 0; // 1 expand the data?
              ierr = g2_getfld(&bu[k], f, unpack, expand, &gfld);

              if (ierr > 0) {
                LogSevere(getGrib2Error(ierr));
              } else {
                if (a != nullptr) { // FIXME: if we're null what's the point of looping?
                  bool keepGoing = a->action(gfld, f);
                  if (keepGoing == false) {
                    g2_free(gfld);
                    return true;
                  }
                }
              }
              g2_free(gfld);
            }
          }

          // End of message match...
        } else { // No hit?  Forget it
          break;
        }
      } else {
        LogSevere("Out of bounds\n");
        break;
      }
    } else {
      LogSevere("No grib2 matched data in buffer\n");
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
{ }

std::shared_ptr<DataType>
IOGrib::readGribDataType(const URL& url)
{
  #if 0
  // g2c 2.0 which we probably won't have forever
  int id;
  // * - ::G2C_NOERROR - No error.
  // * - ::G2C_EINVAL - Invalid input.
  // * - ::G2C_ETOOMANYFILES - Trying to open too many files at the same time.
  auto file = g2c_open(url.toString(), G2C_NOWRITE, &id);
  switch (file) {
      case G2C_NOERROR: LogSevere("NO error\n");
        break;
      case G2C_EINVAL: LogSevere("Invalid input calling g2c_open\n");
        break;
      case G2C_ETOOMANYFILES: LogSevere("Too open grib2 files open at same time on g2c_open\n");
        break;
      default: break;
  }
  g2c_close(id);
  LogSevere("Opened...\n");
  exit(1);
  #endif // if 0

  // HACK IN MY GRIB.data thing for moment...
  // Could lazy read only on string matching...
  GribDatabase::readGribDatabase();

  // Note, in RAPIO we can read a grib file remotely too
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    // FIXME: Validate the format of the GRIB2 file first, then
    // pass it onto the DataType object?
    //  GribSanity test;
    //  if scanGribData(buf, &test) good to go;
    std::shared_ptr<GribDataTypeImp> g = std::make_shared<GribDataTypeImp>(url, buf);
    return g;
  } else {
    LogSevere("Couldn't read data for grib2 at " << url << "\n");
  }

  return nullptr;
} // IOGrib::readGribDataType

std::shared_ptr<DataType>
IOGrib::createDataType(const std::string& params)
{
  // virtual to static, we only handle file/url
  return (IOGrib::readGribDataType(URL(params)));
}

bool
IOGrib::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & params
)
{
  return false;
}

IOGrib::~IOGrib(){ }
