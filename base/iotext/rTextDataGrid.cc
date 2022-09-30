#include "rTextDataGrid.h"

#include "rDataGrid.h"
#include "rError.h"
#include "rConstants.h"

using namespace rapio;
using namespace std;

TextDataGrid::~TextDataGrid()
{ }

void
TextDataGrid::introduceSelf(IOText * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<TextDataGrid>();

  owner->introduce("DataGrid", io);
}

std::shared_ptr<DataType>
TextDataGrid::read(std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>                          dt)
{
  return nullptr;
}

namespace {
void
dumpAttributes(std::shared_ptr<DataAttributeList> list, const std::string& i, const std::string& header = "")
{
  std::ofstream& file = IOText::theFile; // no copy constructor

  for (auto& a:*list) {
    // We do this in netcdf as well, bleh. Is these a cleaner way?
    auto name = a.getName();
    file << i << i << header << ":" << name << " = ";
    if (a.is<std::string>()) {
      file << "\"" << *(a.get<std::string>()) << "\"";
    } else if (a.is<long>()) {
      file << *(a.get<long>()); // get returns optional
    } else if (a.is<float>()) {
      file << *(a.get<float>());
    } else if (a.is<double>()) {
      file << *(a.get<double>());
    } else {
      file << "unknown type";
    }
    file << " ;\n";
  }
}
}

bool
TextDataGrid::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>          & keys)
{
  bool successful = false;

  std::shared_ptr<DataGrid> dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  auto dataType = dataGrid->getDataType();

  // First pass we simulate ncdump on DataGrid
  try{
    std::ofstream& file = IOText::theFile; // no copy constructor

    // Mark standard with RAPIO and datatype
    // This might allow parsing/reading later
    file << "RAPIO/MRMS DataGrid\n";
    const std::string i = "\t";

    // -----------------
    // Write to file
    // First pass just gonna mimic ncdump output.  More work needed
    // FIXME: More general for all rapio formats
    // ------------------------------------------------------------
    // DIMENSIONS
    //
    file << "netcdf {\n";
    file << "dimensions:\n";
    auto dims = dataGrid->getDims();
    for (size_t j = 0; j < dims.size(); ++j) {
      auto& d = dims[j];
      file << "\t" << d.name() << " = " << std::to_string(d.size()) << " ;\n";
    }
    // ------------------------------------------------------------
    // VARIABLES
    //
    file << "variables:\n";
    auto list = dataGrid->getArrays();
    for (auto l:list) {
      // Primary data is the data type of the file
      // Remember functions still take original name
      auto orgName = l->getName();
      auto theName = (orgName == Constants::PrimaryDataName) ? dataGrid->getTypeName() : orgName;

      // Get the variable type such as 'float' to string
      std::string typeStr;
      switch (l->getStorageType()) {
          case BYTE:
            typeStr = "byte";
            break;
          case SHORT:
            typeStr = "short";
            break;
          case INT:
            typeStr = "int";
            break;
          case FLOAT:
            typeStr = "float";
            break;
          case DOUBLE:
            typeStr = "double";
            break;
          default:
            typeStr = "unknown";
            break;
      }

      // Do the dimension strings inside the name:
      // Translate the indexes into the matching netcdf dimension
      file << "\t" << typeStr << " " << theName << "(";
      auto ddims     = l->getDimIndexes();
      const size_t s = ddims.size();
      for (size_t j = 0; j < s; ++j) {
        file << dims[ddims[j]].name();
        if (j != s - 1) { file << ","; }
      }
      file << ") ;\n";

      // Attributes for each variable
      auto att = l->getAttributes();
      dumpAttributes(att, i, theName);
    } // end variables section

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    //
    file << "\n// global attributes:\n";
    auto glist = dataGrid->getGlobalAttributes();
    dumpAttributes(glist, i, "");
    file << "data:\n\n";

    // ------------------------------------------------------------
    // ARRAYS
    for (auto l:list) {
      auto type = l->getStorageType();

      // Primary data is the data type of the file
      // Remember functions still take original name
      auto orgName = l->getName();
      auto theName = (orgName == Constants::PrimaryDataName) ? dataGrid->getTypeName() : orgName;

      // Here I'll break the netcdf 'look' and change a few things for my purposes
      file << " " << theName << " = \n\n";

      // a poor quick wrap around for moment:
      static size_t counter = 1;

      // Conveniently use printArray so we don't have to type things
      auto anArray = dataGrid->getDataArray(orgName);
      if (anArray != nullptr) {
        anArray->printArray(file);
        file << " ;\n";
      } else { // Shouldn't happen..but check anyway
        file << " invalid data.\n";
      }

      #if 0
      // Another option vs printArray would be to get the data template typed and then
      // iterate.  But since type is enforced you'd need this for every array type
      auto dataSP = dataGrid->getFloat1D(theName);
      if (dataSP != nullptr) {
        auto data = dataSP->refAs1D();
        for (auto i = data.begin(); i != data.end(); ++i) {
          file << *i << ", ";
          if (counter++ > 5) { file << "\n"; counter = 0; } // poor person's text wrapper
        }
        file << "\n";
        counter = 1;
      }
      #endif // if 0
    }

    file << "}\n";

    // -----------------
    successful = true;
  }catch (const std::exception& e) {
    LogSevere("Error writing to IOTEXT open file\n");
  }
  return successful;
} // TextDataGrid::write
