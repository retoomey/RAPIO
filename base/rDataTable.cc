#include "rDataTable.h"

using namespace rapio;


bool
PTreeDataTable::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>            & keys)
{
  // We write as generic PTreeData.  Might change later
  // if extra fields go to DataTable
  return false;
}

std::shared_ptr<DataType>
PTreeDataTable::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  // We only introduce as PTreeData
  std::shared_ptr<PTreeData> xml = std::dynamic_pointer_cast<PTreeData>(dt);
  auto datatable = xml->getTree()->getChildOptional("datatable");

  if (datatable != nullptr) {
    auto datatype = datatable->getChildOptional("datatype");
    if (datatype != nullptr) {
      // For now modify/return the original DataType and don't
      // specialize.  Need to figure out writer part
      try{
        // The type (for folder writing)
        const auto theDataType = datatype->getAttr("name", std::string(""));
        xml->setTypeName(theDataType);

        // FIXME: Units assumed secondsSinceEpoch and Degrees/Meters
        // Need to actually convert at some point

        // Time
        auto time        = datatype->getChild("stref.time");
        const auto value = time.getAttr("value", (time_t) (0));
        xml->setTime(Time::SecondsSinceEpoch(value));

        // Location
        auto lat = datatype->getChild("stref.location.lat");
        const auto latDegrees = lat.getAttr("value", double (0));
        auto lon = datatype->getChild("stref.location.lon");
        const auto lonDegrees = lon.getAttr("value", double (0));
        auto ht = datatype->getChild("stref.location.ht");
        const auto htMeters = lon.getAttr("value", double (0));
        xml->setLocation(LLH(latDegrees, lonDegrees, htMeters));
      }catch (const std::exception& e) {
        LogSevere("Tried to read stref tag in datatable and failed: " << e.what() << "\n");
      }
    }
  }

  // Create a specialized DataTable and move the stuff in PTreeData to it.
  std::shared_ptr<DataTable> table = std::make_shared<DataTable>();

  xml->Move(table);
  return table;
} // PTreeDataTable::read
