#include "rTextBinaryTable.h"

#include "rDataGrid.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include "rSignals.h"

using namespace rapio;
using namespace std;

TextBinaryTable::~TextBinaryTable()
{ }

void
TextBinaryTable::introduceSelf(IOText * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<TextBinaryTable>();

  owner->introduce("BinaryTable", io);
  // The specializers here don't make as much sense as other modules,
  // since we call up to the class to print
  // So we could allow BinaryTable to specialize itself
  owner->introduce("WObsBinaryTable", io);
  owner->introduce("RObsBinaryTable", io);
  owner->introduce("Toomey was here", io); // MRMS backward
  owner->introduce("FusionBinaryTable", io);
}

std::shared_ptr<DataType>
TextBinaryTable::read(std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>                             dt)
{
  return nullptr;
}

bool
TextBinaryTable::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys)
{
  bool successful = false;

  std::shared_ptr<BinaryTable> btable = std::dynamic_pointer_cast<BinaryTable>(dt);

  if (btable == nullptr) {
    LogSevere("Not a BinaryTable we can't handle anything else.\n");
    return false;
  }

  try{
    // Text is pretty safe to give to the class itself since it requires
    // no library coupling.
    std::ostream& file = *IOText::theFile; // no copy constructor
    successful = btable->dumpToText(file);
  }catch (const std::exception& e) {
    LogSevere("Error writing to IOTEXT open file\n");
  }
  return successful;
} // TextBinaryTable::write
