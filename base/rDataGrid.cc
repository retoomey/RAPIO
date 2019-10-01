#include "rDataGrid.h"

#include "rError.h"

using namespace rapio;
using namespace std;

std::shared_ptr<RAPIO_1DF>
DataGrid::getFloat1D(const std::string& name)
{
  return myData.get<RAPIO_1DF>(name);
}

void
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, size_t size, float value)
{
  #ifdef BOOST_ARRAY
  auto a = myData.add<RAPIO_1DF>(name, units, boost::multi_array<float, 1>(boost::extents[size]));
  boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  myData.add<RAPIO_1DF>(name, units, DataStore<float>(size, value));
  #endif
}

void
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, size_t size)
{
  #ifdef BOOST_ARRAY
  myData.add<RAPIO_1DF>(name, units, boost::multi_array<float, 1>(boost::extents[size]));
  #else
  myData.add<RAPIO_1DF>(name, units, DataStore<float>(size));
  #endif
}

void
DataGrid::resizeFloat1D(const std::string& name, size_t size, float value)
{
  auto f = myData.get<RAPIO_1DF>(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[size]);
    std::fill(f->begin(), f->end(), value);
    #else
    f->resize(size, value);
    #endif
  } else {
    addFloat1D(name, "Dimensionless", size, value);
  }
}

void
DataGrid::copyFloat1D(const std::string& from, const std::string& to)
{
  auto f = myData.get<RAPIO_1DF>(from);

  if (f != nullptr) {
    size_t aSize = f->size(); // FIXME: I know this can be done better
    resizeFloat1D(to, aSize, 0);
    auto t = myData.get<RAPIO_1DF>(from);
    for (size_t i = 0; i < aSize; ++i) {
      (*t)[i] = (*f)[i];
    }
  }
}

std::shared_ptr<RAPIO_1DI>
DataGrid::getInt1D(const std::string& name)
{
  return myData.get<RAPIO_1DI>(name);
}

void
DataGrid::addInt1D(const std::string& name, const std::string& units, size_t size, int value)
{
  #ifdef BOOST_ARRAY
  auto a = myData.add<RAPIO_1DI>(name, units, boost::multi_array<int, 1>(boost::extents[size]));
  boost::multi_array_ref<int, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  myData.add<RAPIO_1DI>(name, units, DataStore<int>(size, value));
  #endif
}

void
DataGrid::addInt1D(const std::string& name,
  const std::string& units, size_t size)
{
  #ifdef BOOST_ARRAY
  myData.add<RAPIO_1DI>(name, units, boost::multi_array<int, 1>(boost::extents[size]));
  #else
  myData.add<RAPIO_1DI>(name, units, DataStore<int>(size));
  #endif
}

void
DataGrid::resizeInt1D(const std::string& name, size_t size, int value)
{
  auto f = getInt1D(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[size]);
    std::fill(f->begin(), f->end(), value);
    #else
    f->resize(size, value);
    #endif
  } else {
    addInt1D(name, "Dimensionless", size, value);
  }
}

// 2D stuff ----------------------------------------------------------
//

std::shared_ptr<DataNode>
DataGrid::getFloat2DNode(const std::string& name)
{
  return myData.getNode(name);
}

std::shared_ptr<RAPIO_2DF>
DataGrid::getFloat2D(const std::string& name)
{
  return myData.get<RAPIO_2DF>(name);
}

void
DataGrid::addFloat2D(const std::string& name,
  const std::string& units, size_t numx, size_t numy, float value)
{
  #ifdef BOOST_ARRAY
  auto a = myData.add<RAPIO_2DF>(name, units, boost::multi_array<float, 2>(boost::extents[numx][numy]), 0, 1);
  boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);

  #else
  auto a = myData.add<RAPIO_2DF>(name, units, DataStore2D<float>(0, 0), 0, 1);
  a->resize(numx, numy, value);
  #endif
}

void
DataGrid::addFloat2D(const std::string& name,
  const std::string& units, size_t numx, size_t numy)
{
  #ifdef BOOST_ARRAY
  myData.add<RAPIO_2DF>(name, units, boost::multi_array<float, 2>(boost::extents[numx][numy]), 0, 1);
  #else
  myData.add<RAPIO_2DF>(name, units, DataStore2D<float>(numx, numy), 0, 1);
  #endif
}

void
DataGrid::resizeFloat2D(const std::string& name, size_t numx, size_t numy, float value)
{
  auto f = myData.get<RAPIO_2DF>(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[numx][numy]);
    boost::multi_array_ref<float, 1> a_ref(f->data(), boost::extents[f->num_elements()]);
    std::fill(a_ref.begin(), a_ref.end(), value);
    #else
    f->resize(numx, numy, value);
    #endif
  } else {
    addFloat2D(name, "Dimensionless", numx, numy, value);
  }
}

size_t
DataGrid::getY(std::shared_ptr<RAPIO_2DF> f)
{
  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    return f->shape()[1];

    #else
    return f->getY(); // Could implement shape

    #endif
  } else {
    return 0;
  }
}

size_t
DataGrid::getX(std::shared_ptr<RAPIO_2DF> f)
{
  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    return f->shape()[0];

    #else
    return f->getX();

    #endif
  } else {
    return 0;
  }
}

void
DataGrid::replaceMissing(std::shared_ptr<RAPIO_2DF> f, const float missing, const float range)
{
  if (f == nullptr) {
    LogSevere("Replace missing called on null pointer\n");
    return;
  }
  #ifdef BOOST_ARRAY
  // Just view as 1D to fill it...
  boost::multi_array_ref<float, 1> a_ref(f->data(), boost::extents[f->num_elements()]);
  for (size_t i = 0; i < a_ref.num_elements(); ++i) {
    float * v = &a_ref[i];
  #else
  for (auto& i:*f) {
    float * v = &i;
    #endif
    if (*v == missing) {
      *v = Constants::MissingData;
    } else if (*v == range) {
      *v = Constants::RangeFolded;
    }
  }
}

void
DataGrid::resize(size_t numx, size_t numy, const float fill)
{
  resizeFloat2D("primary", numx, numy, fill);
}
