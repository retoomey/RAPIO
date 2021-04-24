#include "rPTreeData.h"

#include "rError.h"
#include "rStrings.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;
using namespace std;

PTreeData::PTreeData()
{
  myRoot     = std::make_shared<PTreeNode>();
  myDataType = "PTREE";
  myTypeName = "PTREE"; // Affects output writing
}
