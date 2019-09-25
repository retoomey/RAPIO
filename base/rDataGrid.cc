#include "rDataGrid.h"

#include "rError.h"

using namespace rapio;
using namespace std;

RAPIO_1DF *
DataGrid::getFloat1D(const std::string& name)
{
  return myFloat1D.get(name);
}

void
DataGrid::addFloat1D(const std::string& name, size_t size, float value)
{
  #ifdef BOOST_ARRAY
  myFloat1D.add(name, boost::multi_array<float, 1>(boost::extents[size]));

  // Fill code separate right? code not good right now
  auto * a = myFloat1D.get(name); // We just made this, slow pull
  boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  myFloat1D.add(name, DataStore<float>(size, value));
  #endif
}

void
DataGrid::addFloat1D(const std::string& name, size_t size)
{
  #ifdef BOOST_ARRAY
  myFloat1D.add(name, boost::multi_array<float, 1>(boost::extents[size]));
  #else
  myFloat1D.add(name, DataStore<float>(size));
  #endif
}

void
DataGrid::resizeFloat1D(const std::string& name, size_t size, float value)
{
  RAPIO_1DF * f = getFloat1D(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[size]);
    fill(value);
    #else
    f->resize(size, value);
    #endif
  } else {
    addFloat1D(name, size, value);
  }
}

RAPIO_1DI *
DataGrid::getInt1D(const std::string& name)
{
  return myInt1D.get(name);
}

void
DataGrid::addInt1D(const std::string& name, size_t size, int value)
{
  #ifdef BOOST_ARRAY
  myInt1D.add(name, boost::multi_array<int, 1>(boost::extents[size]));

  // Not liking this. Slow double allocation
  auto * a = myInt1D.get(name); // We just made this, slow pull
  boost::multi_array_ref<int, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  myInt1D.add(name, DataStore<int>(size, value));
  #endif
}

void
DataGrid::addInt1D(const std::string& name, size_t size)
{
  #ifdef BOOST_ARRAY
  myInt1D.add(name, boost::multi_array<int, 1>(boost::extents[size]));
  #else
  myInt1D.add(name, DataStore<int>(size));
  #endif
}

void
DataGrid::resizeInt1D(const std::string& name, size_t size, int value)
{
  RAPIO_1DI * f = getInt1D(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[size]);
    fill(value);
    #else
    f->resize(size, value);
    #endif
  } else {
    addInt1D(name, size, value);
  }
}

// 2D stuff ----------------------------------------------------------

RAPIO_2DF *
DataGrid::getFloat2D(const std::string& name)
{
  return myFloat2D.get(name);
}

void
DataGrid::addFloat2D(const std::string& name, size_t numx, size_t numy, float value)
{
  #ifdef BOOST_ARRAY
  myFloat2D.add(name, boost::multi_array<float, 2>(boost::extents[numx][numy]));

  // Not liking this, double filling wasteful for large datasets
  auto * a = myFloat2D.get(name);
  boost::multi_array_ref<float, 1> a_ref(a->data(), boost::extents[a->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  myFloat2D.add(name, DataStore2D<float>(0, 0));
  auto * a = myFloat2D.get(name);
  a->resize(numx, numy, value);
  #endif
}

void
DataGrid::addFloat2D(const std::string& name, size_t numx, size_t numy)
{
  #ifdef BOOST_ARRAY
  myFloat2D.add(name, boost::multi_array<float, 2>(boost::extents[numx][numy]));
  #else
  myFloat2D.add(name, DataStore2D<float>(numx, numy));
  #endif
}

void
DataGrid::resizeFloat2D(const std::string& name, size_t numx, size_t numy, float value)
{
  auto * f = myFloat2D.get(name);

  if (f != nullptr) {
    #ifdef BOOST_ARRAY
    f->resize(boost::extents[numx][numy]);
    fill(value);
    #else
    f->resize(numx, numy, value); // BACKWARDS STILL
    #endif
  } else {
    addFloat2D(name, numx, numy, value);
  }
}

void
DataGrid::set(size_t i, size_t j, const float& v)
{
  // Note resize should have called once
  auto * f = myFloat2D.get("primary");

  if (f == nullptr) {
    LogSevere("Set called null pointer\n");
    return;
  }
  #ifdef BOOST_ARRAY
  (*f)[i][j] = v;
  #else
  (*f).set(i, j, v);
  #endif
}

void
DataGrid::fill(const float& value)
{
  // Note resize should have called once
  auto * f = myFloat2D.get("primary");

  if (f == nullptr) {
    LogSevere("Set called null pointer\n");
    return;
  }
  #ifdef BOOST_ARRAY
  // Just view as 1D to fill it...
  boost::multi_array_ref<float, 1> a_ref(f->data(), boost::extents[f->num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), value);
  #else
  std::fill(f->begin(), f->end(), value);
  #endif
}

size_t
DataGrid::getY()
{
  auto * f = myFloat2D.get("primary");

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
DataGrid::getX()
{
  auto * f = myFloat2D.get("primary");

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
DataGrid::replaceMissing(const float missing, const float range)
{
  auto * f = myFloat2D.get("primary");

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
