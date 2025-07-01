#include "rIOWgrib.h"
#include "rWgribFieldImp.h"
#include "rWgribDataTypeImp.h"

#include <rError.h>

using namespace rapio;

WgribFieldImp::WgribFieldImp(const URL& url,
  int messageNumber,
  int fieldNumber,
  long int filePos,
  std::array<long, 3>& sec0, std::array<long, 13>& sec1) :
  myURL(url), GribField(messageNumber, fieldNumber)
{
  mySection0 = sec0;
  mySection1 = sec1;
}

WgribFieldImp::~WgribFieldImp()
{ }

long
WgribFieldImp::getDisciplineNumber()
{
  return mySection0[0];
}

long
WgribFieldImp::getGRIBEditionNumber()
{
  return mySection0[1];
}

#if 0
long
WgribFieldImp::getMessageLength()
{
  return mySection0[2];
}

#endif

size_t
WgribFieldImp::getGridDefTemplateNumber()
{
  // FIXME: Do we really need this?
  return 0;
}

size_t
WgribFieldImp::getSigOfRefTime()
{
  // FIXME: Do we really need this?
  return 0;
}

Time
WgribFieldImp::getTime()
{
  Time theTime = Time(mySection1[5], // year
      mySection1[6],                 // month
      mySection1[7],                 // day
      mySection1[8],                 // hour
      mySection1[9],                 // minute
      mySection1[10],                // second
      0.0                            // fractional
  );

  return theTime;
}

std::ostream&
WgribFieldImp::print(std::ostream& os)
{
  return os;
}

std::string
WgribFieldImp::getProductName()
{
  return "???";
}

std::string
WgribFieldImp::getLevelName()
{
  return "???";
}

std::shared_ptr<Array<float, 2> >
WgribFieldImp::getFloat2D()
{
  LogSevere("getFloat2D not implemented in field class\n");
  return nullptr;
} // WgribFieldImp::getFloat2D

std::shared_ptr<Array<float, 3> >
WgribFieldImp::getFloat3D(std::shared_ptr<Array<float, 3> > in, size_t atZ, size_t numZ)
{
  LogSevere("getFloat3D not implemented in field class\n");
  return nullptr;
} // WgribFieldImp::getFloat3D
