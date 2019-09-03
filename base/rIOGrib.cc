#include "rIOGrib.h"

#include "rFactory.h"
#include "rBuffer.h"
#include "rIOURL.h"

#include "rGribDataType.h"

extern "C" {
#include <grib2.h>
}

using namespace rapio;

void
grib2err(g2int ierr)
{
  switch (ierr) {
      case 0: LogSevere("No Error\n");
        break;
      case 1: LogSevere("Beginning characters GRIB not found\n");
        break;
      case 2: LogSevere("Grib message is not Edition 2\n");
        break;
      case 3: LogSevere("Could not find Section 1, where expected\n");
        break;
      case 4: LogSevere("End string '7777' found, but not where expected\n");
        break;
      case 5: LogSevere("End string '7777' not found at end of message\n");
        break;
      case 6: LogSevere("Invalid section number found\n");
        break;
      default:
        LogSevere("Unknown grib2 error: " << ierr << "\n");
        break;
  }
}

void
myinfobuf(unsigned char * bu, size_t at)
{
  // unsigned char* bu = (unsigned char*)(&b[0]);
  g2int listsec0[3], listsec1[13], numlocal;
  g2int myNumFields = 0;

  int ierr = g2_info(&bu[at], &listsec0[0], &listsec1[0], &myNumFields, &numlocal);

  if (ierr > 0) {
    grib2err(ierr);
  } else {
    LogSevere("Grib info: " << myNumFields << "\n");
    for (size_t dd = 0; dd < 3; ++dd) {
      LogSevere("---head: " << (int) (listsec0[dd]) << "\n");
    }
    for (size_t dd = 5; dd < 11; ++dd) {
      LogSevere("---cal: " << (int) (listsec1[dd]) << "\n");
    }
  }
}

void
myseekgbbuf(Buffer& b, g2int iseek, g2int mseek, g2int * lskip, g2int * lgrib)
{
  g2int k4, start, vers, lengrib;
  int end;

  start  = 0;
  vers   = 0;
  *lgrib = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //  LOOP UNTIL GRIB MESSAGE IS FOUND
  size_t aSize       = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);
  LogSevere("GRIB2 incoming: buffer size " << aSize << "\n");

  // Seekgb is actually kinda simple, it finds a single grib2 message and its
  // length into a FILE*.  We want to use a buffer though..this gives us the
  // advantage of being able to use a URL/ram, etc.
  // We store a lookup table of messages/lengths on full pass.  Allows
  // algorithms to 'pull' only the data they want
  // seekdb.c has the c code for this from a FILE*.  The downside
  // to us doing this is that we need to check future versions of seekdb.c
  // for bug fixes/changes.
  size_t messageCount = 0;
  size_t k = 0;
  while (k < aSize) {
    gbit(bu, &start, (k + 0) * 8, 4 * 8);
    gbit(bu, &vers, (k + 7) * 8, 1 * 8);

    if (( start == 1196575042) && (( vers == 1) || ( vers == 2) )) {
      //  LOOK FOR '7777' AT END OF GRIB MESSAGE
      if (vers == 1) { gbit(bu, &lengrib, (k + 4) * 8, 3 * 8); }
      if (vers == 2) { gbit(bu, &lengrib, (k + 12) * 8, 4 * 8); }
      //    ret=fseek(lugb,ipos+k+lengrib-4,SEEK_SET);
      const size_t at = k + lengrib - 4;
      if (at + 3 < aSize) {
        k4 = 1;
        //    k4=fread(&end,4,1,lugb); ret fails overload
        end = int((unsigned char) (b[at]) << 24
          | (unsigned char) (b[at + 1]) << 16
          | (unsigned char) (b[at + 2]) << 8
          | (unsigned char) (b[at + 3]));
        if (( k4 == 1) && ( end == 926365495) ) { // GRIB message found
          messageCount++;
          // LogSevere("FOUND GRIB BUFFER MESSAGE! " << end << " k = " << k << "\n");
          // *lskip=ipos+k;  // Position into buffer for message start.
          //           LogSevere("Message position: " << k << "\n");
          // *lgrib=lengrib; // Length of the Grib2 message
          //           LogSevere("Message length: " << lengrib << "\n");


          // Do g2_info on this message section
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
            grib2err(ierr);
          } else {
            // Can check center first before info right?
            g2int cntr = listsec1[0]; // Id of orginating centre
            g2int mtab = listsec1[2]; // =GRIB Master Tables Version Number (Code Table 1.0)

            const size_t fieldNum = (myNumFields < 0) ? 0 : (size_t) (myNumFields);
            for (size_t f = 1; f <= fieldNum; ++f) {
              gribfield * gfld = 0; // output
              g2int unpack     = 1; // 0 do not unpack
              g2int expand     = 1; // 1 expand the data?
              int ierr         = g2_getfld(&bu[k], f, unpack, expand, &gfld);

              if (ierr > 0) {
                grib2err(ierr);
              } else {
                // Key1: Grib2 discipline is also in the header, but we can get it here
                g2int disc = gfld->discipline;
                // Grib2 Grid Definition Template (GDT) number.  This tells how the grid is projected...
                const g2int gdtn = gfld->igdtnum;

                // Grib2 PRODUCT definition template number.  Tells you which Template 4.x defines
                // the stuff in the ipdtmpl.  Technically we would have to check all of these to
                // make sure the Product Category is the first number...
                int pdtn = gfld->ipdtnum;

                // Key 2: Category  Section 4, Octet 10 (For all Templates 4.X)
                g2int pcat = gfld->ipdtmpl[0];

                // Key 3: Parameter Number.  Section 4, Octet 11
                g2int pnum = gfld->ipdtmpl[1];

                // We have to match what data we want somehow...

                /*
                 * int disc;   Section 0 Discipline // field
                 * int mtab;   Section 1 Master Tables Version Number
                 * int cntr;   Section 1 originating centre, used for local tables // higher
                 * int ltab;   Section 1 Local Tables Version Number
                 * int pcat;   Section 4 Template 4.0 Parameter category // field
                 * int pnum;   Section 4 Template 4.0 Parameter number // field
                 */
                //   { 0, 1, 0, 0, 5, 4, "ULWRF", "Upward Long-Wave Rad. Flux", "W/m^2"},
                //   { 0, 0, 7, 1, 5, 193, "ULWRF", "Upward Long-Wave Rad. Flux", "W/m^2"}
                // if ((mtab == 1) || (mtab == 0)){ // 2nd
                if ((cntr == 0) || (cntr == 7)) { // 3rd
                  if (disc == 0) {                // 1th
                    // 1 1 1
                    if ((pcat == 5) ) {                   // 5th
                      if ((pnum == 4) || (pnum == 193)) { // 6th
                        LogSevere(
                          " Discipline: " << (int) disc
                                          << " Master Tab: " << (int) mtab
                                          << " Center: " << (int) cntr
                                          << " Category: " << pcat
                                          << " ParameterNum: " << pnum
                                          <<// Projection information.  Will be needed at some point.
                            " Projection: " << (int) gdtn
                                          << " PRODUCT: " << (int) pdtn
                                          << "\n");
                      }
                    }
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
    //   break;
  }

  LogSevere("Total messages: " << messageCount << "\n");
} // myseekgbbuf

/** A local test function for me */
void
IOGrib::test()
{
  URL loc("/home/dyolf/GRIB2/test.grib2");
  Buffer buf;

  if (IOURL::read(loc, buf)) {
    // FILE * fptr = 0; // crash is fine
    FILE * fptr = fopen("/home/dyolf/GRIB2/test.grib2", "rb");

    g2int myiseek = 0;
    g2int mylgrib = 0;
    g2int mymseek = 32000;
    g2int mylskip = 0;

    // Make our own seekgb so we can use a stream not a file pointer.
    // I really wish most of these types of libraries would have APIs
    // for in memory processing rather than file only access.  For example,
    // netcdf _just_ recently added that ability.

    // for(size_t zz=0; zz< 10000; ++zz){
    LogSevere("CALLING OLD SEEK  " << mylskip << ", " << mylgrib << "\n");
    seekgb(fptr, myiseek, mymseek, &mylskip, &mylgrib);
    //   myiseek = mylskip + mylgrib;
    LogSevere(">>>>>>>>>>>>>>>>>>> BACK " << mylskip << ", " << mylgrib << "\n");

    std::vector<unsigned char> mycgrib;
    mycgrib.resize(mylgrib);

    LogSevere("SEEK TO " << mylskip << "\n");
    int ret = fseek(fptr, mylskip, SEEK_SET);
    if (ret < 0) {
      LogSevere("fseek failed reading grib2 file\n");
      exit(1);
    }

    size_t read = fread(&(mycgrib[0]), sizeof(mycgrib[0]), mycgrib.size(), fptr);
    LogSevere("ACTUAL READ " << read << "\n");
    for (size_t pp = 0; pp < 20; ++pp) {
      LogSevere(pp << " " << (int) (mycgrib[pp]) << "\n");
    }

    myiseek = mylskip + mylgrib;

    g2int listsec0[3], numlocal, mylistsec1[13];
    g2int myNumFields = 0;
    int ierr = g2_info(&(mycgrib[0]), listsec0, mylistsec1, &myNumFields,
        &numlocal);
    if (ierr < 0) {
      LogSevere("g2_info in grib2 failed.\n");
      exit(1);
    }
    LogSevere("Got back " << listsec0[0] << ", " << listsec0[1] << "\n");
    LogSevere("Grib info: " << myNumFields << "\n");
    for (size_t dd = 0; dd < 3; ++dd) {
      LogSevere("---head: " << (listsec0[dd]) << "\n");
    }
    for (size_t dd = 5; dd < 11; ++dd) {
      LogSevere("---cal: " << (mylistsec1[dd]) << "\n");
    }
    if (mylgrib == 0) {
      LogSevere("BREAKING OLD\n");
      exit(1);
    }

    LogSevere("CALLING NEW SEEK -------------------------\n");

    LogSevere("Read success!\n");
    exit(1);
  }
  exit(1);
} // IOGrib::test

void
IOGrib::introduceSelf()
{
  std::shared_ptr<IOGrib> newOne = std::make_shared<IOGrib>();
  Factory<IODataType>::introduce("grib", newOne);

  // Add the default classes we handle, if any
}

std::shared_ptr<DataType>
IOGrib::readGribDataType(const std::vector<std::string>& args)
{
  // Note, in RAPIO we can read a grib file remotely too
  const URL url = getFileName(args);
  Buffer buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    g2int iseek = 0;
    g2int lgrib = 0;
    g2int mseek = 32000;
    g2int lskip = 0;

    // Validate the format of the GRIB2 file first, then
    // pass it onto the DataType object
    LogSevere("READ " << url << "\n");
    myseekgbbuf(buf, iseek, mseek, &lskip, &lgrib);
    // GribDataType d = make
    // FIXME: If I do this, HOW much goes into the data type vs the factory?
    // I think the basic read/check for valid data should be in reader...
    // only return datatype iff the data file is valid file format
    //
    std::shared_ptr<GribDataType> g = std::make_shared<GribDataType>(buf);
    return g;
  } else {
    LogSevere("NOT READ " << url << "\n");
  }


  // return (IONetcdf::readNetcdfDataType(args));
  LogSevere(">>>>>>>>>>>>>>>>CALLED CREATE OBJECT YAY\n");
  for (auto i:args) {
    LogSevere("ARG: " << i << "\n");
  }
  test();
  return nullptr;
} // IOGrib::readGribDataType

std::shared_ptr<DataType>
IOGrib::createObject(const std::vector<std::string>& args)
{
  // virtual to static
  return (IOGrib::readGribDataType(args));
}

std::string
IOGrib::writeGribDataType(const rapio::DataType& dt,
  const std::string                            & myDirectory,
  std::shared_ptr<DataFormatSetting>           dfs,
  std::vector<Record>                          & records)
{
  return "";
}

std::string
IOGrib::encode(const rapio::DataType & dt,
  const std::string                  & directory,
  std::shared_ptr<DataFormatSetting> dfs,
  std::vector<Record>                & records)
{
  return (IOGrib::writeGribDataType(dt, directory, dfs, records));
}

IOGrib::~IOGrib(){ }
