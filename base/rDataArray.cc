#include "rDataArray.h"
#include "rError.h"

using namespace rapio;
using namespace std;

std::shared_ptr<DataArray>
DataArray::Clone()
{
  auto n = std::make_shared<DataArray>();

  // Copy all the array attributes (including units)
  n->myAttributes = myAttributes->Clone();

  n->myName        = getName();
  n->myStorageType = getStorageType();
  n->myDimIndexes  = getDimIndexes();

  // Now copy stored array (We don't need to know the specialization)
  n->myArray = myArray->CloneBase();
  n->setArray(n->myArray, n->myArray);

  return n;
}
