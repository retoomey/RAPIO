#include "rArray.h"

using namespace rapio;
using namespace std;

// Array class, everything in header as template class.

std::shared_ptr<Array<float, 1> >
Arrays::CreateFloat1D(size_t x)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  std::shared_ptr<Array<float, 1> > a =
    std::make_shared<Array<float, 1> >(dims);

  return a;
}

std::shared_ptr<Array<int, 1> >
Arrays::CreateInt1D(size_t x)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  std::shared_ptr<Array<int, 1> > a =
    std::make_shared<Array<int, 1> >(dims);

  return a;
}

std::shared_ptr<Array<float, 2> >
Arrays::CreateFloat2D(size_t x, size_t y)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  dims.push_back(y);
  std::shared_ptr<Array<float, 2> > a =
    std::make_shared<Array<float, 2> >(dims);

  return a;
}

std::shared_ptr<Array<int, 2> >
Arrays::CreateInt2D(size_t x, size_t y)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  dims.push_back(y);
  std::shared_ptr<Array<int, 2> > a =
    std::make_shared<Array<int, 2> >(dims);

  return a;
}

std::shared_ptr<Array<float, 3> >
Arrays::CreateFloat3D(size_t x, size_t y, size_t z)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  dims.push_back(y);
  dims.push_back(z);
  std::shared_ptr<Array<float, 3> > a =
    std::make_shared<Array<float, 3> >(dims);

  return a;
}

std::shared_ptr<Array<int, 3> >
Arrays::CreateInt3D(size_t x, size_t y, size_t z)
{
  std::vector<size_t> dims;

  dims.push_back(x);
  dims.push_back(y);
  dims.push_back(z);
  std::shared_ptr<Array<int, 3> > a =
    std::make_shared<Array<int, 3> >(dims);

  return a;
}
