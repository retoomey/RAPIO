#include "rNetcdfRadialSet.h"

#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"
#include "rProcessTimer.h"

#include <netcdf.h>

#include <rDataGrid.h> // macros

using namespace rapio;
using namespace std;

NetcdfRadialSet::~NetcdfRadialSet()
{ }

void
NetcdfRadialSet::introduceSelf()
{
  std::shared_ptr<NetcdfType> io = std::make_shared<NetcdfRadialSet>();

  // NOTE: We read in polar grids and turn them into full radialsets
  IONetcdf::introduce("RadialSet", io);
  IONetcdf::introduce("PolarGrid", io);
  IONetcdf::introduce("SparseRadialSet", io);
  IONetcdf::introduce("SparsePolarGrid", io);
}

std::shared_ptr<DataType>
NetcdfRadialSet::read(const int ncid, const URL& loc,
  const vector<string>& params)
{
  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>();
  if (readDataGrid(ncid, radialSetSP, loc, params)) {
    return radialSetSP;
  } else {
    return nullptr;
  }
} // NetcdfRadialSet::read

bool
NetcdfRadialSet::write(int ncid, std::shared_ptr<DataType> dt,
  std::shared_ptr<DataFormatSetting> dfs)
{
  // FIXME: Note, we might want to validate the dimensions, etc.
  // Two dimensions: "Azimuth", "Gate"
  return (NetcdfDataGrid::write(ncid, dt, dfs));
} // NetcdfRadialSet::write

std::shared_ptr<DataType>
NetcdfRadialSet::getTestObject(
  LLH    location,
  Time   time,
  size_t objectNumber)
{
  const size_t num_radials = 100;
  const size_t num_gates   = 20;
  const float elev         = 0.50;
  const double elev_angle  = elev;
  const float gate_width   = 1;
  const float beam_width   = 2;

  // float radial_time        = 1000.0; // Add a second to radial time for each
  //                                   // radial...

  // std::string nyq_unit("MetersPerSecond");

  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>(location, time, Length());
  RadialSet& radialSet = *radialSetSP;

  radialSet.setTypeName("Reflectivity");
  radialSet.setElevation(elev_angle);

  // Allow in-line vector storage to work and not crash
  // radialSet.resize(num_radials, num_gates);
  // radialSet.declareDims({ num_radials, num_gates }, {"Azimuth", "Gates"});
  radialSet.init(num_radials, num_gates);
  // radialSet.setNyquistVelocityUnit(nyq_unit);

  auto azimuths   = radialSet.getFloat1D("Azimuth");
  auto beamwidths = radialSet.getFloat1D("BeamWidth");
  auto gatewidths = radialSet.getFloat1D("GateWidth");

  auto data = radialSet.getFloat2D("primary");
  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree
    (*azimuths)[i]   = start_az;
    (*beamwidths)[i] = beam_width;
    (*gatewidths)[i] = gate_width;
    for (size_t j = 0; j < num_gates; ++j) {
      (*data)[i][j] = i;
    }
  }

  radialSet.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");
  return (radialSetSP);
} // NetcdfRadialSet::getTestObject
