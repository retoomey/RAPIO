#include "rTextPTreeData.h"
#include "rError.h"
#include "rIOJSON.h"

using namespace rapio;

void
TextPTreeData::introduceSelf(IOText * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<TextPTreeData>();

  owner->introduce("PTreeData", io);
  // Use superclass write ability for now
  owner->introduce("ProbSevere", io);
  owner->introduce("PTREE", io);
}

std::shared_ptr<DataType>
TextPTreeData::read(IOConfig& config)
{
  return nullptr; // Text reading is typically unsupported for dumps
}

bool
TextPTreeData::write(std::shared_ptr<DataType> dt, IOConfig& keys)
{
  auto ptree = std::dynamic_pointer_cast<PTreeData>(dt);

  if (!ptree) {
    fLogSevere("Not a PTreeData object.");
    return false;
  }

  try {
    std::ostream& o = *IOText::theFile;

    o << "RAPIO PTreeData Dump\n";
    o << "DataType: " << ptree->getDataType() << "\n";
    o << "TypeName: " << ptree->getTypeName() << "\n";
    o << "Time:     " << ptree->getTime().getString() << "\n";
    o << "Location: " << ptree->getLocation() << "\n";
    o << std::string(60, '-') << "\n";

    // Use the base IOJSON utility to dump the property tree to a formatted string
    std::vector<char> buf;
    if ((IOJSON::writePTreeDataBuffer(ptree, buf) > 0) && !buf.empty()) {
      o.write(buf.data(), buf.size() - 1);
    } else {
      o << "[Empty PTreeData]\n";
    }

    o << "\n";
    return true;
  } catch (const std::exception& e) {
    fLogSevere("Error writing PTreeData text dump: {}", e.what());
    return false;
  }
} // TextPTreeData::write
