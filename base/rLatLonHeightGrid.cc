#include "rLatLonHeightGrid.h"
#include "rProject.h"
#include "rProcessTimer.h"

using namespace rapio;
using namespace std;

std::string
LatLonHeightGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonHeightGrid::LatLonHeightGrid() : myLatSpacing(0), myLonSpacing(0)
{
  myDataType = "LatLonHeightGrid";
}

std::shared_ptr<LatLonHeightGrid>
LatLonHeightGrid::Create(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the each 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 3D array
  size_t num_lats,
  size_t num_lons,
  size_t num_layers
)
{
  auto llgridsp = std::make_shared<LatLonHeightGrid>();

  if (llgridsp == nullptr) {
    LogSevere("Couldn't create LatLonHeightGrid\n");
  } else {
    // We post constructor fill in details because many of the factories like netcdf 'chain' layers and settings
    llgridsp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons, num_layers);
  }

  return llgridsp;
}

void
LatLonHeightGrid::init(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & location,
  const Time       & time,
  const float      lat_spacing,
  const float      lon_spacing,
  size_t           num_lats,
  size_t           num_lons,
  size_t           num_layers
)
{
  setTypeName(TypeName);
  setDataAttributeValue("Unit", "dimensionless", Units);

  myLocation   = location;
  myTime       = time;
  myLatSpacing = lat_spacing;
  myLonSpacing = lon_spacing;
  myNumLats    = num_lats;
  myNumLons    = num_lons;

  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    LogSevere("*** WARNING *** non-positive element in grid spacing\n"
      << "(" << lat_spacing << "," << lon_spacing << ")\n");
  }

  // Ok 'how' we implement...for the moment we're doing N 2D LatLonGrids,
  // but if 3D we would declareDims with 3 dimensions
  // With our N 2D implementation we can lazy implement it I think.
  // Update primary grid to the given size
  // Which funny enough means there's actual no data directly within us.
  LogSevere("----pre 3D creation..\n");

  {
    ProcessTimer timeit("Creating as a 3D all at once\n");
    declareDims({ num_lats, num_lons, num_layers }, { "Lat", "Lon", "H" });
    addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });

    LogSevere("----post 3D creation..\n");
    LogSevere("Time post 3D is now " << timeit << "\n");
  }

  // Note layers random...you need to fill them all during data reading
  myLayerNumbers.resize(num_layers);
  myLatLonGrids.resize(num_layers); // These will be nullptr
} // LatLonHeightGrid::init

std::shared_ptr<LatLonGrid>
LatLonHeightGrid::getLatLonGrid(size_t layerNumber)
{
  if (layerNumber >= myLayerNumbers.size()) {
    LogSevere(
      "Requestion layer number " << layerNumber << " which is out of range in LatLonHeightGrid 0 to " <<
        myLayerNumbers.size());
    return nullptr;
  }
  auto out = myLatLonGrids[layerNumber];

  if (out == nullptr) {
    std::string units;
    getString("Unit", units);
    // Create a new LatLonGrid matching our stats
    out = LatLonGrid::Create(
      getTypeName(),
      units,
      //     varName,
      //     varUnit,
      myLocation, // Humm the height is initially the first passed in LatLonHeightGrid height
                  // Up to callar to update the height?  We don't know 100% that the layer number
      // is a height right?  I kinda want to be able to do things that aren't height as
      // well like s-layers
      // If our technique works reasonably at some point we'll generalize for
      // CartesianGrids maybe...they only differ by not marching in lat/lon which is
      // pretty minor
      myTime,
      myLatSpacing,
      myLonSpacing,
      myNumLats, // num_lats
      myNumLons  // num_lons
    );
    //
  }
  myLatLonGrids[layerNumber] = out;
  LogSevere("    " << myNumLats << ", " << myNumLons << "\n");

  return nullptr;
} // LatLonHeightGrid::getLatLonGrid

// void setLayerValue(size_t layerNumber, int layerValue)
// {
// }

bool
LatLonHeightGrid::initFromGlobalAttributes()
{
  bool success = true;

  // Shared coded with RadialSet.  Gotta be careful moving 'up'
  // since the DataGrid can read general netcdf which could be
  // missing this information.  Still thinking about it.

  // TypeName check, such as Reflectivity or Velocity
  if (!getString(Constants::TypeName, myTypeName)) {
    LogSevere("Missing TypeName attribute such as Reflectivity.\n");
    success = false;
  }

  // -------------------------------------------------------
  // Location
  float latDegs  = 0;
  float lonDegs  = 0;
  float htMeters = 0;

  success &= getFloat(Constants::Latitude, latDegs);
  success &= getFloat(Constants::Longitude, lonDegs);
  success &= getFloat(Constants::Height, htMeters);
  if (success) {
    myLocation = LLH(latDegs, lonDegs, htMeters / 1000.0);
  } else {
    LogSevere("Missing Location attribute\n");
  }

  // -------------------------------------------------------
  // Time
  long timesecs = 0;

  if (getLong(Constants::Time, timesecs)) {
    // optional
    double f = 0.0;
    getDouble(Constants::FractionalTime, f);
    // Cast long to time_t..hummm
    myTime = Time(timesecs, f);
  } else {
    LogSevere("Missing Time attribute\n");
    success = false;
  }

  // LatLonHeightGrid only

  // -------------------------------------------------------
  // Latitude and Longitude grid spacing
  if (!getFloat("LatGridSpacing", myLatSpacing)) {
    LogSevere("Missing LatGridSpacing attribute\n");
    success = false;
  }
  if (!getFloat("LonGridSpacing", myLonSpacing)) {
    LogSevere("Missing LonGridSpacing attribute\n");
    success = false;
  }

  return success;
} // LatLonHeightGrid::initFromGlobalAttributes

void
LatLonHeightGrid::updateGlobalAttributes(const std::string& encoded_type)
{
  // Note: Datatype updates the attributes -unit -value specials,
  // so don't add any after this
  DataType::updateGlobalAttributes(encoded_type);

  // LatLonHeightGrid only global attributes
  setDouble("LatGridSpacing", myLatSpacing);
  setDouble("LonGridSpacing", myLonSpacing);
}

std::shared_ptr<DataProjection>
LatLonHeightGrid::getProjection(const std::string& layer)
{
  return std::make_shared<LatLonHeightGridProjection>(layer, this);
}
