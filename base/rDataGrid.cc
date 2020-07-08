#include "rDataGrid.h"
#include "rTime.h"
#include "rLLH.h"

#include "rError.h"

using namespace rapio;
using namespace std;

LLH
DataGrid::getLocation() const
{
  return LLH();
}

Time
DataGrid::getTime() const
{
  return Time();
}

void *
DataArray::getRawDataPointer()
{
  // Bypass all type safety for reader/writer convenience
  // Returning pointer to any of possible stored array types
  // Sadly boost etc doesn't seem to have a common superclass,
  // maybe we could rework this.
  // FIXME: maybe keep a weak ref copy of the pointer on creation
  const size_t size = myDimIndexes.size();

  if (myStorageType == FLOAT) {
    if (size == 2) {
      // return (getSP<RAPIO_2DF>()->data());
      return (getSP<Array<float, 2> >()->getRawDataPointer());
    }
    if (size == 1) {
      // return (getSP<RAPIO_1DF>()->data());
      return (getSP<Array<float, 1> >()->getRawDataPointer());
    }
  }
  if (myStorageType == INT) {
    if (size == 1) {
      // return (getSP<RAPIO_1DI>()->data());
      return (getSP<Array<int, 1> >()->getRawDataPointer());
    }
  }

  return nullptr;
}

std::shared_ptr<DataAttributeList>
DataArray::getAttributes()
{
  return myAttributes;
}

std::shared_ptr<DataAttributeList>
DataGrid::getAttributes(
  const std::string& name)
{
  auto node = getNode(name);

  if (node != nullptr) {
    return node->getAttributes();
  }
  return nullptr;
}

std::shared_ptr<DataArray>
DataGrid::getDataArray(const std::string& name)
{
  return getNode(name);
}

void
DataGrid::declareDims(const std::vector<size_t>& dimsizes,
  const std::vector<std::string>               & dimnames)
{
  // Store updated dimension info
  myDims.clear();
  for (size_t zz = 0; zz < dimsizes.size(); ++zz) {
    myDims.push_back(DataGridDimension(dimnames[zz], dimsizes[zz]));
  }

  // Resize all arrays to given dimensions....
  for (auto l:myNodes) {
    auto i    = l->getDimIndexes();
    auto name = l->getName();
    auto type = l->getStorageType();
    auto size = i.size();

    // From each index into dimension, get actual sizes
    // i ==> 0, 1 or say 1, 0 if flipped
    // to --> 50, 100 size for example 0--> 50, 1 --> 100
    std::vector<size_t> sizes(size);
    for (size_t d = 0; d < size; ++d) {
      sizes[d] = dimsizes[i[d]];
    }

    // Since we want convenience classes and well boost
    // arrays don't have a common superclass, some
    // type checking here.  FIXME: better way?
    if (size == 1) {
      if (type == FLOAT) {
        auto f = l->getSP<Array<float, 1> >();
        if (f != nullptr) {
          f->resize({ sizes[0] });
        }
      } else if (type == INT) {
        auto f = l->getSP<Array<int, 1> >();
        if (f != nullptr) {
          f->resize({ sizes[0] });
        }
      }
    } else if (size == 2) {
      auto f = l->getSP<Array<float, 2> >();
      if (f != nullptr) {
        f->resize({ sizes[0], sizes[1] });
      }
    }
  }
} // DataGrid::declareDims

// 1D stuff ----------------------------------------------------------
//

std::shared_ptr<Array<float, 1> >
DataGrid::getFloat1D(const std::string& name)
{
  return get<Array<float, 1> >(name);
}

std::shared_ptr<Array<float, 1> >
DataGrid::addFloat1D(const std::string& name,
  const std::string& units, const std::vector<size_t>& dimindexes)
{
  // Map index to dimension sizes
  const auto size = myDims[dimindexes[0]].size();
  auto a = add<Array<float, 1> >(name, units, Array<float, 1>({ size }), FLOAT, dimindexes);

  return a;
}

std::shared_ptr<Array<int, 1> >
DataGrid::getInt1D(const std::string& name)
{
  return get<Array<int, 1> >(name);
}

std::shared_ptr<Array<int, 1> >
DataGrid::addInt1D(const std::string& name,
  const std::string& units, const std::vector<size_t>& dimindexes)
{
  // Map index to dimension sizes
  const auto size = myDims[dimindexes[0]].size();
  auto a = add<Array<int, 1> >(name, units, Array<int, 1>({ size }), INT, dimindexes);

  return a;
}

// 2D stuff ----------------------------------------------------------
//

std::shared_ptr<Array<float, 2> >
DataGrid::getFloat2D(const std::string& name)
{
  return get<Array<float, 2> >(name);
}

std::shared_ptr<Array<float, 2> >
DataGrid::addFloat2D(const std::string& name,
  const std::string& units, const std::vector<size_t>& dimindexes)
{
  // Map index to dimension sizes
  const auto x = myDims[dimindexes[0]].size();
  const auto y = myDims[dimindexes[1]].size();

  // We always add/replace since index order could change or it could be different
  // type, etc.  Maybe possibly faster by updating over replacing
  auto a = add<Array<float, 2> >(name, units, Array<float, 2>({ x, y }), FLOAT, dimindexes);

  return a;
}

void
DataGrid::replaceMissing(std::shared_ptr<Array<float, 2> > f, const float missing, const float range)
{
  if (f == nullptr) {
    LogSevere("Replace missing called on null pointer\n");
    return;
  }
  // #ifdef BOOST_ARRAY
  // Just view as 1D to fill it...
  LogSevere(">>>REPLACE MISSING NOT IMPLEMENTED\n");

  /*
   * boost::multi_array_ref<float, 1> a_ref(f->data(), boost::extents[f->num_elements()]);
   * for (size_t i = 0; i < a_ref.num_elements(); ++i) {
   *  float * v = &a_ref[i];
   * }
   */

  /*
   #else
   * for (auto& i:*f) {
   *  float * v = &i;
   #endif
   *  if (*v == missing) {
   * v = Constants::MissingData;
   *  } else if (*v == range) {
   * v = Constants::RangeFolded;
   *  }
   */
}
