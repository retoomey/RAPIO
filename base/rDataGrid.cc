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
DataGrid::declareDims(const std::vector<size_t>& dims)
{
  myData.declareDims(dims);

  // Resize all arrays to given dimensions....
  auto list = getArrays();
  for (auto l:list) {
    auto i    = l->getDims();
    auto name = l->getName();
    auto type = l->getStorageType();
    auto size = i.size();

    // Messy resize.
    // FIXME: Can we generialze this more?
    // Could template it more, but I kinda want
    // students/users to get a defined array type back
    if (size == 1) {
      if (type == FLOAT) {
        auto f = l->get<RAPIO_1DF>();
        if (f != nullptr) {
          const size_t size = dims[i[0]];
          #ifdef BOOST_ARRAY
          f->resize(boost::extents[size]);
          #else
          f->resize(size);
          #endif
        }
      } else if (type == INT) {
        auto f = l->get<RAPIO_1DI>();
        if (f != nullptr) {
          const size_t size = dims[i[0]];
          #ifdef BOOST_ARRAY
          f->resize(boost::extents[size]);
          #else
          f->resize(size);
          #endif
        }
      }
    } else if (size == 2) {
      auto f = l->get<RAPIO_2DF>();
      if (f != nullptr) {
        const size_t numx = dims[i[0]];
        const size_t numy = dims[i[1]];
        #ifdef BOOST_ARRAY
        f->resize(boost::extents[numx][numy]);
        //   boost::multi_array_ref<float, 1> a_ref(f->data(), boost::extents[f->num_elements()]);
        //   std::fill(a_ref.begin(), a_ref.end(), value);
        #else
        f->resize(numx, numy);
        #endif
      }
    }
  }
} // DataGrid::declareDims

std::shared_ptr<RAPIO_1DF>
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, const std::vector<size_t>& dims)
{
  auto a = myData.get<RAPIO_1DF>(name);

  if (a == nullptr) {
    auto d    = myData.getDims(); // Dimensions with sizes...
    auto size = d[dims[0]];       // So basically dims is the index into dimensions
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DF>(name, units, boost::multi_array<float, 1>(boost::extents[size]), FLOAT);
    #else
    a = myData.add<RAPIO_1DF>(name, units, DataStore<float>(size), FLOAT);
    #endif
  }
  return a;
}

/*
 * void
 * DataGrid::copyFloat1D(const std::string& from, const std::string& to)
 * {
 * auto f = myData.get<RAPIO_1DF>(from);
 *
 * if (f != nullptr) {
 *  size_t aSize = f->size(); // FIXME: I know this can be done better
 *  resizeFloat1D(to, aSize, 0);
 *  auto t = myData.get<RAPIO_1DF>(from);
 *  for (size_t i = 0; i < aSize; ++i) {
 *    (*t)[i] = (*f)[i];
 *  }
 * }
 * }
 */

std::shared_ptr<RAPIO_1DI>
DataGrid::getInt1D(const std::string& name)
{
  return myData.get<RAPIO_1DI>(name);
}

std::shared_ptr<RAPIO_1DI>
DataGrid::addInt1D(const std::string& name,
  const std::string& units, const std::vector<size_t>& dims)
{
  auto a = myData.get<RAPIO_1DI>(name);

  if (a == nullptr) {
    auto d    = myData.getDims(); // Dimensions with sizes...
    auto size = d[dims[0]];       // So basically dims is the index into dimensions
    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_1DI>(name, units, boost::multi_array<int, 1>(boost::extents[size]), INT);
    #else
    a = myData.add<RAPIO_1DI>(name, units, DataStore<int>(size), INT);
    #endif
  }
  return a;
}

/*
 * void
 * DataGrid::resizeInt1D(const std::string& name, size_t size)
 * {
 * auto f = getInt1D(name);
 *
 * if (f != nullptr) {
 #ifdef BOOST_ARRAY
 *  f->resize(boost::extents[size]);
 * //    std::fill(f->begin(), f->end(), value);
 #else
 *  //f->resize(size, value);
 *  f->resize(size);
 #endif
 * } else {
 *  //addInt1D(name, "Dimensionless", {0}, value);
 *  addInt1D(name, "Dimensionless", {0});
 * }
 * }
 */

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
  const std::string& units, const std::vector<size_t>& dims)
{
  auto a = myData.get<RAPIO_2DF>(name);

  if (a == nullptr) {
    auto d    = myData.getDims(); // Dimensions with sizes...
    auto numx = d[dims[0]];       // So basically dims is the index into dimensions
    auto numy = d[dims[1]];

    #ifdef BOOST_ARRAY
    a = myData.add<RAPIO_2DF>(name, units, boost::multi_array<float, 2>(boost::extents[numx][numy]), FLOAT, { 0, 1 });

    // std::fill(a_ref.begin(), a_ref.end(), value);


    #else
    a = myData.add<RAPIO_2DF>(name, units, DataStore2D<float>(numx, numy), FLOAT, { 0, 1 });
    #endif
  } else {
    // Might have already been added..update the fields.  FIXME: check conflict?
    // Well crap...double fetchin not liking design.  Step by step
    std::shared_ptr<DataNode> n = myData.getNode(name);
    n->setUnits(units);
    n->setDims({ 0, 1 }); // Always in dim order at least for now.
    n->setStorageType(FLOAT);
  }
  return a;
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

/*
 * void
 * DataGrid::resize(size_t numx, size_t numy, const float fill)
 * {
 * resizeFloat2D("primary", numx, numy, fill);
 * }
 */
