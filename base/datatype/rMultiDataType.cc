#include "rMultiDataType.h"

using namespace rapio;

std::shared_ptr<MultiDataType>
MultiDataType::Create(bool groupWrite)
{
  auto newonesp = std::make_shared<MultiDataType>(groupWrite);

  // Why not just make a shared ptr?  Well we might
  // end up having to initialize other things
  // TODO: Add anu thing else needed
  return newonesp;
}

std::shared_ptr<DataType>
MultiDataType::Simplify(std::shared_ptr<MultiDataType>& m)
{
  if (m) {
    const size_t s = m->size();
    switch (s) {
        case 0: return nullptr;

        case 1: return m->getDataType(0);

        default: return m;
    }
  }
  return nullptr;
}

void
MultiDataType::addDataType(std::shared_ptr<DataType> add)
{
  // On first add, set read factory to match the first one,
  // this will route stuff say to netcdf if it was read that
  // way.
  if (myDataTypes.size() == 0) {
    setReadFactory(add->getReadFactory());
  }
  // FIXME: Do we check if we already have it?
  myDataTypes.push_back(add);
}

std::shared_ptr<DataType>
MultiDataType::getDataType(size_t i)
{
  if (i < myDataTypes.size()) {
    return myDataTypes[i];
  } else {
    return nullptr;
  }
}

size_t
MultiDataType::size()
{
  return myDataTypes.size();
}
