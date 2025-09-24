#include "rGribFieldImp.h"
#include "rGribDatabase.h"
#include "rIOGrib.h"
#include "rGribDataTypeImp.h"

#include <rError.h>

using namespace rapio;

GribFieldImp::~GribFieldImp()
{
  /** Free our internal grib pointer if we have one */
  if (myGribField != nullptr) {
    g2_free(myGribField);
  }
}

bool
GribFieldImp::needsToReload(bool unpacked, bool expanded) const
{
  // We reload if null or if the unpacked/expanded increases
  // in level.
  bool reload = false;

  if (myGribField == nullptr) {
    reload = true;
  } else {
    // If we move 'up' in unpacked/expanded flag requirement,
    // then we want to do the long/slow read
    if ((unpacked && !myUnpacked) || (expanded && !myExpanded)) {
      reload = true;
    }
  }
  return reload;
}

bool
GribFieldImp::fieldLoaded(bool unpacked, bool expanded)
{
  if (needsToReload(unpacked, expanded)) {
    // Get rid of old one.
    if (myGribField != nullptr) {
      g2_free(myGribField);
      myGribField = nullptr;
    }

    // Load new one.  The myBufferPtr might need the GribDataTypeImp, but
    // not 'always'.  For now, assume if GribDataTypeImp went out of scope
    // that we cannot load.
    if (myDataTypeValid.lock() == nullptr) {
      LogSevere("GribDataType is no longer valid.  Grib fields and Grib messages require it to exist.\n");
      return false;
    }
    const g2int unpack = unpacked ? 1 : 0; // do/do not unpack
    const g2int expand = expanded ? 1 : 0; // do/do not expand the data?
    myUnpacked = unpacked;
    myExpanded = expanded;
    int ierr = g2_getfld(myBufferPtr, myFieldNumber, unpack, expand, &myGribField);

    if (ierr > 0) {
      LogSevere(IOGrib::getGrib2Error(ierr));
      myGribField = nullptr; // Does g2_getfld delete on error?
    }
  }
  return (myGribField != nullptr);
} // GribFieldImp::fieldLoaded

long
GribFieldImp::getGRIBEditionNumber()
{
  if (fieldLoaded()) {
    return (myGribField->version);
  }
  return 0;
}

long
GribFieldImp::getDisciplineNumber()
{
  if (fieldLoaded()) {
    return (myGribField->discipline);
  }
  return 0;
}

size_t
GribFieldImp::getGridDefTemplateNumber()
{
  if (fieldLoaded()) {
    return (myGribField->igdtnum);
  }
  return 0;
}

size_t
GribFieldImp::getSigOfRefTime()
{
  if (fieldLoaded()) {
    auto& f = *myGribField;
    return f.idsect[4];
  }
  return 0;
}

Time
GribFieldImp::getTime()
{
  if (fieldLoaded()) {
    auto& f      = *myGribField;
    Time theTime = Time(f.idsect[5], // year
        f.idsect[6],                 // month
        f.idsect[7],                 // day
        f.idsect[8],                 // hour
        f.idsect[9],                 // minute
        f.idsect[10],                // second
        0.0                          // fractional
    );
    return theTime;
  }
  return Time();
}

std::ostream&
GribFieldImp::print(std::ostream& os)
{
  if (fieldLoaded()) {
    std::string productName = getProductName();
    std::string levelName   = getLevelName();
    os << "d=" << getDateString() << ":" << productName << ":" << levelName << "\n";
  }
  return os;
}

std::string
GribFieldImp::getProductName()
{
  if (fieldLoaded()) {
    std::string product;
    auto data = myDataTypeValid.lock();
    if (data && data->myDataType->getIDXProductName(myMessageNumber, myFieldNumber, product)) { } else {
      product = GribDatabase::getProductName(myGribField);
    }
    return product;
  } else {
    return "???";
  }
}

std::string
GribFieldImp::getLevelName()
{
  if (fieldLoaded()) {
    std::string level;
    auto data = myDataTypeValid.lock();
    if (data && data->myDataType->getIDXLevelName(myMessageNumber, myFieldNumber, level)) { } else {
      level = GribDatabase::getLevelName(myGribField);
    }
    return level;
  } else {
    return "???";
  }
}

std::shared_ptr<Array<float, 2> >
GribFieldImp::getFloat2D()
{
  if (!fieldLoaded(true, true)) { // full load
    LogSevere("Couldn't unpack/expand field data!\n");
    return nullptr;
  }

  LogInfo("Grib2 field unpack/expand successful.\n");
  auto& f = *myGribField;
  g2float * g2grid = f.fld;

  if (g2grid == nullptr) {
    LogSevere("Internal grid float point was nullptr, nothing read into array.\n");
    return nullptr;
  }

  // Dimensions
  const g2int * gds = f.igdtmpl;

  const size_t numLon = (gds[7] < 0) ? 0 : (size_t) (gds[7]); // 31-34 Nx -- Number of points along the x-axis (W-E)
  const size_t numLat = (gds[8] < 0) ? 0 : (size_t) (gds[8]); // 35-38 Ny -- Number of points along the y-axis (N-S)

  // Keep the dimensions
  LogInfo("Grib2 2D field size: " << numLat << " (lat) * " << numLon << " (lon).\n");


  // Fill array with 2D data
  auto newOne = Arrays::CreateFloat2D(numLat, numLon); // MRMS ordering
  auto& data  = newOne->ref();

  for (size_t lat = 0; lat < numLat; ++lat) {
    for (size_t lon = 0; lon < numLon; ++lon) {
      const size_t flipLat = numLat - (lat + 1);
      data[lat][lon] = (float) (g2grid[(flipLat * numLon) + lon]);
    }
  }
  return newOne;
} // GribFieldImp::getFloat2D

std::shared_ptr<Array<float, 3> >
GribFieldImp::getFloat3D(std::shared_ptr<Array<float, 3> > in, size_t atZ, size_t numZ)
{
  if (!fieldLoaded(true, true)) { // full load
    LogSevere("Couldn't unpack/expand field data!\n");
    return in;
  }

  LogInfo("Grib2 field unpack/expand successful.\n");
  auto& f = *myGribField;
  g2float * g2grid = f.fld;

  if (g2grid == nullptr) {
    LogSevere("Internal grid float point was nullptr, nothing read into array.\n");
    return nullptr;
  }

  // Dimensions
  const g2int * gds = f.igdtmpl;

  const size_t numLon = (gds[7] < 0) ? 0 : (size_t) (gds[7]); // 31-34 Nx -- Number of points along the x-axis (W-E)
  const size_t numLat = (gds[8] < 0) ? 0 : (size_t) (gds[8]); // 35-38 Ny -- Number of points along the y-axis (N-S)

  // Fill a single Z layer with 2D data

  // Use given array or if we're the first, create it with out dimensions
  std::shared_ptr<Array<float, 3> > the3D;

  if (in == nullptr) {
    // We're the first, create the 3D array
    LogInfo("Grib2 3D field size: " << numLat << " (lat) * " << numLon << " (lon) * " << numZ << " levels.\n");
    the3D = Arrays::CreateFloat3D(numLat, numLon, numZ);
  } else {
    auto& s = in->getSizes();
    // Check given old array matches our dimension, otherwise we can't append
    if ((s[0] != numLat) || (s[1] != numLon) || (s[2] != numZ)) {
      LogSevere("Mismatch on secondary layer dimensions, can't add 2D layer (" << atZ << ")  "
                                                                               << numLat << " != " << s[0] << " or "
                                                                               << numLon << " != " << s[1] << " or "
                                                                               << numZ << " != " << s[2] << "\n");
      return in;
    }
    the3D = in;
  }

  // Fill layer with 2D data
  auto& data = the3D->ref();

  for (size_t lat = 0; lat < numLat; ++lat) {
    for (size_t lon = 0; lon < numLon; ++lon) {
      const size_t flipLat = numLat - (lat + 1);
      data[lat][lon][atZ] = (float) (g2grid[(flipLat * numLon) + lon]);
    }
  }

  return the3D;
} // GribFieldImp::getFloat3D
