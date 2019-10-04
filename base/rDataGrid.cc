#include "rDataGrid.h"

#include "rError.h"

using namespace rapio;
using namespace std;

std::shared_ptr<RAPIO_1DF>
DataGrid::getFloat1D(const std::string& name)
{
  return myData.get<RAPIO_1DF>(name);
}

std::shared_ptr<RAPIO_1DF>
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, size_t size, float value)
{
  auto a = myData.get<RAPIO_1DF>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DF>(name, units, boost::multi_array<float, 1>(boost::extents[size]));
    boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
    std::fill(a_ref.begin(), a_ref.end(), value);
    #else
    a = myData.add<RAPIO_1DF>(name, units, DataStore<float>(size, value));
    #endif
  }
  return a;
}

std::shared_ptr<RAPIO_1DF>
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, size_t size)
{
  auto a = myData.get<RAPIO_1DF>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DF>(name, units, boost::multi_array<float, 1>(boost::extents[size]));
    #else
    a = myData.add<RAPIO_1DF>(name, units, DataStore<float>(size));
    #endif
  }
  return a;
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

std::shared_ptr<RAPIO_1DI>
DataGrid::addInt1D(const std::string& name, const std::string& units, size_t size, int value)
{
  auto a = myData.get<RAPIO_1DI>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DI>(name, units, boost::multi_array<int, 1>(boost::extents[size]));
    boost::multi_array_ref<int, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
    std::fill(a_ref.begin(), a_ref.end(), value);
    #else
    a = myData.add<RAPIO_1DI>(name, units, DataStore<int>(size, value));
    #endif
  }
  return a;
}

std::shared_ptr<RAPIO_1DI>
DataGrid::addInt1D(const std::string& name,
  const std::string& units, size_t size)
{
  auto a = myData.get<RAPIO_1DI>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DI>(name, units, boost::multi_array<int, 1>(boost::extents[size]));
    #else
    a = myData.add<RAPIO_1DI>(name, units, DataStore<int>(size));
    #endif
  }
  return a;
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
DataGrid::getDataNode(const std::string& name)
{
  return myData.getNode(name);
}

std::shared_ptr<RAPIO_2DF>
DataGrid::getFloat2D(const std::string& name)
{
  return myData.get<RAPIO_2DF>(name);
}

std::shared_ptr<RAPIO_2DF>
DataGrid::addFloat2D(const std::string& name,
  const std::string& units, size_t numx, size_t numy, float value)
{
  auto a = myData.get<RAPIO_2DF>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_2DF>(name, units, boost::multi_array<float, 2>(boost::extents[numx][numy]), 0, 1);
    boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
    std::fill(a_ref.begin(), a_ref.end(), value);

    #else
    a = myData.add<RAPIO_2DF>(name, units, DataStore2D<float>(0, 0), 0, 1);
    a->resize(numx, numy, value);
    #endif
  }
  return a;
}

std::shared_ptr<RAPIO_2DF>
DataGrid::addFloat2D(const std::string& name,
  const std::string& units, size_t numx, size_t numy)
{
  auto a = myData.get<RAPIO_2DF>(name);

  if (a == nullptr) {
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_2DF>(name, units, boost::multi_array<float, 2>(boost::extents[numx][numy]), 0, 1);
    #else
    a = myData.add<RAPIO_2DF>(name, units, DataStore2D<float>(numx, numy), 0, 1);
    #endif
  }
  return a;
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
