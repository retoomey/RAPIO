#include "rArray.h"

using namespace rapio;
using namespace std;

// Array class, everything in header as template class.

std::shared_ptr<Array<float, 1> >
Arrays::CreateFloat1D(size_t x, float v)
{
  std::vector<size_t> dims;
  dims.push_back(x);
  std::shared_ptr<Array<float, 1> > a =
    std::make_shared<Array<float, 1> >(dims, v);
  return a;
}

std::shared_ptr<Array<float, 2> >
Arrays::CreateFloat2D(size_t x, size_t y, float v)
{
  std::vector<size_t> dims;
  dims.push_back(x);
  dims.push_back(y);
  std::shared_ptr<Array<float, 2> > a =
    std::make_shared<Array<float, 2> >(dims, v);
  return a;
}

std::shared_ptr<Array<float, 3> >
Arrays::CreateFloat3D(size_t x, size_t y, size_t z, float v)
{
  std::vector<size_t> dims;
  dims.push_back(x);
  dims.push_back(y);
  dims.push_back(z);
  std::shared_ptr<Array<float, 3> > a =
    std::make_shared<Array<float, 3> >(dims, v);
  return a;
}
