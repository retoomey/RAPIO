#include "rIOWgrib.h"
#include "rWgribFieldImp.h"
#include "rWgribDataTypeImp.h"

#include <rError.h>

using namespace rapio;

WgribFieldImp::~WgribFieldImp()
{ }

long
WgribFieldImp::getGRIBEditionNumber()
{
  return 0;
}

long
WgribFieldImp::getDisciplineNumber()
{
  return 0;
}

size_t
WgribFieldImp::getGridDefTemplateNumber()
{
  return 0;
}

size_t
WgribFieldImp::getSigOfRefTime()
{
  return 0;
}

Time
WgribFieldImp::getTime()
{
  return Time();
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
  return nullptr;
} // WgribFieldImp::getFloat2D

std::shared_ptr<Array<float, 3> >
WgribFieldImp::getFloat3D(std::shared_ptr<Array<float, 3> > in, size_t atZ, size_t numZ)
{
  return nullptr;
} // WgribFieldImp::getFloat3D
