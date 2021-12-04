#include "rNetcdfRadialSet.h"

#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include <netcdf.h>

#include <rDataGrid.h> // macros

using namespace rapio;
using namespace std;

NetcdfRadialSet::~NetcdfRadialSet()
{ }

void
NetcdfRadialSet::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NetcdfRadialSet>();

  // NOTE: We read in polar grids and turn them into full radialsets
  owner->introduce("RadialSet", io);
  owner->introduce("PolarGrid", io);
  owner->introduce("SparseRadialSet", io);
  owner->introduce("SparsePolarGrid", io);
}

std::shared_ptr<DataType>
NetcdfRadialSet::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>();

  if (readDataGrid(radialSetSP, keys)) {
    return radialSetSP;
  } else {
    return nullptr;
  }
} // NetcdfRadialSet::read

bool
NetcdfRadialSet::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys)
{
  // FIXME: Note, we might want to validate the dimensions, etc.
  // Two dimensions: "Azimuth", "Gate"
  return (NetcdfDataGrid::write(dt, keys));
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

  float firstGateDistanceMeters = 1000.0;
  auto radialSetSP = RadialSet::Create("Reflectivity", "dbZ", location, time,
      elev_angle, firstGateDistanceMeters, num_radials, num_gates);
  RadialSet& radialSet = *radialSetSP;
  // radialSet.setNyquistVelocityUnit(nyq_unit);

  auto azimuthsA   = radialSet.getFloat1D("Azimuth");
  auto& azimuths   = azimuthsA->ref();
  auto beamwidthsA = radialSet.getFloat1D("BeamWidth");
  auto& beamwidths = beamwidthsA->ref();
  auto gatewidthsA = radialSet.getFloat1D("GateWidth");
  auto& gatewidths = gatewidthsA->ref();

  auto array = radialSet.getFloat2D("primary");
  auto& data = array->ref();

  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree
    azimuths[i]   = start_az;
    beamwidths[i] = beam_width;
    gatewidths[i] = gate_width;
    for (size_t j = 0; j < num_gates; ++j) {
      data[i][j] = i;
    }
  }

  // radialSet.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");
  return (radialSetSP);
} // NetcdfRadialSet::getTestObject
