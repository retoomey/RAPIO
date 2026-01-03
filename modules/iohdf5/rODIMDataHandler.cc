#include "rODIMDataHandler.h"
#include "rConfigODIMProductInfo.h"
#include "rHDF5Group.h"
#include "rHDF5Attribute.h"
#include "rHDF5DataSet.h"
#include "rHDF5DataSpace.h"
#include "rRadialSet.h"
#include "rStrings.h"

using namespace rapio;
using namespace std;
using namespace hdflib;

namespace // anonymous
{
// ----------------------------------------------------------------------------
// Parse radar name from the /what/source attribute value.
//
// Currently only parses NOD Identifier type as detailed on page 11 of the
// ODIM_H5 2.2 guide.
// FIXME: Future iterations probably need to add logic to parse
// WMO, RAD, PLC, ORG, CTY, and CMT Identifier types.
//
// @param[in] t_source   string containing value of /what/source attribute
//                       described in table 1 page 10 of ODIM_H5 2.2 guide.
//                       Example: NOD:casbe,PLC:Bethune SK
// @returns std::string containing parsed radar name
// ----------------------------------------------------------------------------
std::string
parseRadarName(const std::string& t_source)
{
  std::string radarName("");

  /// Node Source Type based on table 3 page 11 in ODIM_H5 2.2 guide)
  std::string sourceType("NOD:");
  const char sourceTypeDelim(',');

  // Starting position for source type containing radar name
  size_t currentPos = t_source.find(sourceType);

  // Found the source key so let's get the value next.
  if (currentPos != std::string::npos) {
    std::size_t radarBegPos = currentPos + sourceType.size();
    currentPos = t_source.find(sourceTypeDelim, radarBegPos);
    radarName  = t_source.substr(radarBegPos, currentPos - radarBegPos);

    // Strip spaces from radar name.
    radarName.erase(0, radarName.find_first_not_of(' '));

    // Strip trailing spaces from string
    radarName.erase(radarName.find_last_not_of(' ') + 1);

    // Upper-case the radar name (standard in MRMS)
    Strings::toUpper(radarName);
  }
  return radarName;
} // end of std::string parseRadarName(const std::string& t_source)
} // namespace anonymous

ODIMDataHandler::~ODIMDataHandler()
{ }

void
ODIMDataHandler::introduceSelf(IOHDF5 * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<ODIMDataHandler>();

  owner->introduce("ODIM", io);
}

std::shared_ptr<DataType>
ODIMDataHandler::read(std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>                             dt)
{
  // Options in RAPIO are passed by keys, allowing algs or binaries
  // to override/add ability
  const hid_t hdf5id         = std::stoll(keys["HDF5_ID"]);
  const std::string filename = keys["HDF5_URL"];

  // FIXME: Hesitating to break into classes for the moment
  // I'd like to implement the CVOL as well?  Might be useful
  //
  // Micheal had some basic classes ODIMRadar, Sweep, Moment.which may
  // be cleaner.

  try{
    double beamWidth(1.0); // Beamwidth should default to 1 degree if missing.

    // --------------------------------------------------------------------
    // ODIM required data
    // It expects the "/"
    HDF5Group root(hdf5id, "/"); // throws
    std::string m_convention;
    root.getAttribute("Conventions", m_convention);
    LogDebug("/ (Conventions): " << m_convention << "\n"); // to API?

    // Find out what we're working with
    auto what = root.getSubGroup("what");
    std::string odimObject;
    what.getAttribute("object", odimObject);
    LogDebug("/what/object (ODIM File Object): " << odimObject << '\n');

    // Get source
    std::string sourceName("Unknown");
    what.getAttribute("source", sourceName);
    sourceName = parseRadarName(sourceName);

    // While the beamwidth attribute is optional - it is nice that Canada sends
    // it on all radars. There are other radar systems that do not send the
    // beamwidth and therefore need to account for those times with some method
    // to hard-code the attribute.
    auto how = root.getSubGroup("how"); // no id required, maybe cleaner
    how.getAttribute("beamwidth", &beamWidth);
    LogDebug("/how/beamwidth (Beamwidth): " << beamWidth << '\n');

    // where
    LLH location;
    auto where = root.getSubGroup("where");
    double latitudeDegs(0.0);
    where.getAttribute("lat", &latitudeDegs);
    double longitudeDegs(0.0);
    where.getAttribute("lon", &longitudeDegs);
    double heightMeters(0.0);
    where.getAttribute("height", &heightMeters);
    location = LLH(latitudeDegs, longitudeDegs, heightMeters / 1000.0);

    // --------------------------------------------------------------------
    // Dispatcher.  FIXME: Add a factory maybe and deligation classes
    //

    /* Object types
     * PVOL Polar volume
     * CVOL Cartesian volume
     * SCAN Polar scan
     * RAY Single polar ray
     * AZIM Azimuthal object
     * ELEV Elevational object
     * IMAGE 2-D cartesian image
     * COMP Cartesian composite image(s)
     * XSEC 2-D vertical cross section(s)
     * VP 1-D vertical profile
     * PIC Embedded graphical image
     */

    Strings::toUpper(odimObject);
    if ((odimObject == "SCAN") || (odimObject == "PVOL")) {
      return readODIM_SCANPVOL(hdf5id, beamWidth, location, sourceName);
    } else {
      LogSevere("HDF5 ODIM is type '" << odimObject << "', which is unimplemented\n");
    }
  }catch (const std::exception& e) {
    LogSevere("HDF5 file doesn't appear to be ODIM, can't read it.\n");
    return nullptr;
  }

  return nullptr;
} // ODIMDataHandler::read

std::shared_ptr<DataType>
ODIMDataHandler::readODIM_SCANPVOL(hid_t hdf5id, double beamWidth, LLH location,
  const std::string& sourceName)
{
  // Debating.  Grib2 files are so big we 'pull' a field by name from it.
  // it gets its own data type and an index ability. We could use this
  // ability with ODIN volumes as well.  The advantage in that API is
  // that large files only read the requested index DataType.
  //
  // This technique here 'auto reads all' basically, which may be bad
  // for extremely large files.  We'll try because I think both have their
  // place. So for ODIM PVOL files we will read everything for now.
  // This will allow rcopy to turn one ODIM file into multiple outputs.
  // Another addition with this technique would be to pass a key as a filter.
  //
  auto m = MultiDataType::Create();

  // Get each dataset in the hdf5.
  size_t index = 1;

  while (true) {
    std::string datasetName = "dataset" + std::to_string(index++);
    if (HDF5Group::hasGroup(hdf5id, datasetName)) {
      HDF5Group dataset(hdf5id, datasetName);
      readODIM_SCANPVOL_DATASET(m, dataset, beamWidth, location, sourceName);
    } else {
      break;
    }
  }

  // Reduce to single DataType or keep complicated if needed
  return MultiDataType::Simplify(m);
}

bool
ODIMDataHandler::readODIM_SCANPVOL_DATASET(std::shared_ptr<MultiDataType>& output,
  HDF5Group& dataset, double beamWidth, LLH location, const std::string& sourceName)
{
  auto what = dataset.getSubGroup("what");

  // -------------------------------------------------------
  // FIXME: General time method?
  //
  // FIXME: Why startdate and not enddate?
  std::string startDate("");

  what.getAttribute("startdate", startDate);
  std::string startTime("");

  what.getAttribute("starttime", startTime);

  startDate += startTime;
  Time time;

  try {
    time = Time(startDate, "%Y%m%d%H%M%S");
  }catch (const std::exception& e) {
    LogSevere(e.what() << "\n");
    LogSevere("Can't read date/time, using current time. \n");
    time = Time::CurrentTime();
  }

  // --------------------------------------------------------------------
  // DATASET#/WHERE: algate, elangle, nbins, nrays, rscale, rstart
  //
  auto where = dataset.getSubGroup("where");

  size_t m_a1gate(0); /**< Index of 1st gate radiated */

  where.getAttribute("a1gate", &m_a1gate);
  LogDebug("/dataset#/where/a1gate (Index to 1st radiated gate in sweep): " << m_a1gate << '\n');

  double m_elevationDegs(0); /**< Elevation (degrees) of sweep */

  where.getAttribute("elangle", &m_elevationDegs);
  LogDebug("/dataset#/where/elangle (Sweep Elevation): " << m_elevationDegs << '\n');

  size_t m_binCount(0); /**< Count of bins (Gates) in sweep */

  where.getAttribute("nbins", &m_binCount);
  LogDebug("/dataset#/where/nbins (Number of bins in sweep): " << m_binCount << '\n');

  size_t m_rayCount(0); /**< Count of rays (Azimuth) in sweep */

  where.getAttribute("nrays", &m_rayCount);
  LogDebug("/dataset#/where/nrays (Number of rays in sweep): " << m_rayCount << '\n');

  double m_gateWidthMeters(0); /**< Range (width) between gates */

  where.getAttribute("rscale", &m_gateWidthMeters);
  LogDebug("/dataset#/where/rscale (Width of gate): " << m_gateWidthMeters << '\n');

  double m_gate1RangeKMs(0); /**< Range to ray's 1st gate */

  where.getAttribute("rstart", &m_gate1RangeKMs);
  LogDebug("/dataset#/where/rstart (Range to ray's 1st Gate): " << m_gate1RangeKMs << '\n');

  // --------------------------------------------------------------------
  // DATASET#/HOW
  //
  auto how = dataset.getSubGroup("how");

  const auto g = how.getId();

  std::vector<double> m_azimuthStartAngles; /**< Starting Azimuth angles in sweep */

  getAttributeValues(g, "startazA", m_azimuthStartAngles);

  std::vector<double> m_azimuthTimes; /**< Starting Azimuth times in sweep */

  getAttributeValues(g, "startazT", m_azimuthTimes);
  // There 'is' a stopazA.  Maybe can use that for spacing calculation?

  size_t m_scanIndex(0); /**< Temporal sequence # of scan (1-based) */

  how.getAttribute("scan_index", &m_scanIndex);
  LogDebug("/dataset#/how/scan_index (Index of 1st Sweep in volume): " << m_scanIndex << '\n');

  size_t m_scanCount(0); /**< Number of scans  in volume */

  how.getAttribute("scan_count", &m_scanCount);
  LogDebug("/dataset#/how/scan_count (Number of scans in volume): " << m_scanCount << '\n');

  // Get Azimuthal starting offset in degrees from 0 of the first ray in sweep.
  double m_azimuthStartOffset(0); /**< Starting azimuth offset from 0 */

  how.getAttribute("astart", &m_azimuthStartOffset);
  LogDebug("/dataset#/how/astart (Azimuthal offset from 1st ray): " << m_azimuthStartOffset << '\n');

  double m_nyquistVelocity(0); /**< Nyquist Velocity for sweep */

  how.getAttribute("NI", &m_nyquistVelocity);
  LogDebug("/dataset#/how/NI (Nyquist Velocity): " << m_nyquistVelocity << '\n');

  // Capture the radar status message.
  std::string m_radarMsg(""); /**< Status Message for sweep */

  how.getAttribute("radar_msg", m_radarMsg);

  // Determine if radar is having issues.
  bool m_isMalfunction(false); /**< Radar malfunctioned indicator */
  std::string isMalfunction("False");

  how.getAttribute("malfunc", isMalfunction);
  m_isMalfunction = (isMalfunction == "True");

  // --------------------------------------------------------------------
  // The moments...
  //
  size_t index = 1;

  while (true) {
    std::string dataName = "data" + std::to_string(index++);
    if (dataset.hasSubGroup(dataName)) {
      auto data = dataset.getSubGroup(dataName);
      readODIM_MOMENT(output, data, beamWidth, location, time,
        sourceName,
        m_elevationDegs, m_gate1RangeKMs, m_gateWidthMeters,
        m_rayCount, m_binCount, m_a1gate, m_azimuthStartAngles,
        m_nyquistVelocity, m_radarMsg, m_isMalfunction);
    } else {
      break;
    }
  }
  return true;
} // ODIMDataHandler::read

bool
ODIMDataHandler::readODIM_MOMENT(
  std::shared_ptr<MultiDataType>& output,
  HDF5Group& data,
  double beamWidth, LLH location, Time time,
  const std::string& sourceName,
  double m_elevationDegs,
  double m_gate1RangeKMs, double m_gateWidthMeters, size_t m_rayCount,
  size_t m_binCount, size_t m_a1gate, std::vector<double>& m_azimuthStartAngles,
  double m_nyquistVelocity, const std::string& radarMsg, bool isMalfunction)
{
  auto what = data.getSubGroup("what");

  // Defaulting to 1 per ODIM_H5 guidelines page 21.
  double m_gainCoeff(1.0); /**< Coefficient a in y=ax+b conversion */

  what.getAttribute("gain", &m_gainCoeff);
  LogDebug("ODIM_H5 gain coefficient 'a' in y=ax+b: " << m_gainCoeff << '\n');

  // ODIM_H5 'nodata' attribute is MRMS-equivalent of Constants::DataUnavailable
  // Typical 'nodata' value found in ODIM_H5 file is 255 or 65535
  double m_nodataValue(0.0); /**< Default value for no data in sweep */

  what.getAttribute("nodata", &m_nodataValue);
  LogDebug("ODIM_H5 default value for 'nodata': " << m_nodataValue << '\n');

  // Defaulting to 0 per OIMD_H5 guidelines page 21.
  double m_offsetCoeff(0.0); /**< Coefficient b in y=ax+b conversion */

  what.getAttribute("offset", &m_offsetCoeff);
  LogDebug("ODIM_H5 offset coefficient 'b' in y=ax+b: " << m_offsetCoeff << '\n');

  std::string m_name(""); /**< Moment name or Product name (quantity)*/

  what.getAttribute("quantity", m_name);
  LogDebug("Moment name: " << m_name << '\n');

  // ODIM_H5 'undetect' attribute is MRMS-equivalent of Constants::MissingData
  // Typical 'undetect' value found in ODIM_H5 file is 0
  double m_undetectValue(0.0); /**< Default value for undetected values */

  what.getAttribute("undetect", &m_undetectValue);
  LogDebug("DEBUG: ODIM_H5 default value for 'undetect': " << m_undetectValue << '\n');

  // --------------------------------------------------------------------
  // Now data1/data..
  //
  HDF5DataSet hdf5Dataset(data.getId(), "data");
  HDF5DatasetSpace datasetSpace(hdf5Dataset.getId());
  const size_t maxDimensionsAllowed(2); // RadialSets are 2D.  Can we have a 3D ODIM RadialSet?
  const size_t rank = datasetSpace.getRank();

  if (rank != maxDimensionsAllowed) {
    throw std::runtime_error("Unsupported dataspace (more than two-dimension)");
  }
  const std::vector<hsize_t> dimSizes = datasetSpace.getDimensions(rank);

  size_t m_dataRayCount;     /**< Count of Rays in sweep */
  size_t m_dataBinCount;     /**< Count of gates (bins) in sweep */
  size_t m_dataElementCount; /**< Count of raw values in sweep */

  m_dataRayCount = static_cast<size_t>(dimSizes[0]); // compare to m_rayCount, right?
  LogDebug("Number of Rays: " << m_dataRayCount << '\n');

  m_dataBinCount = static_cast<size_t>(dimSizes[1]);
  LogDebug("Number of Bins: " << m_dataBinCount << '\n');

  m_dataElementCount = datasetSpace.getElementCount();
  LogDebug("Number of Elements: " << m_dataElementCount << '\n');

  if (m_dataElementCount != (m_dataRayCount * m_dataBinCount)) {
    throw std::runtime_error("Issue with dataspace (element count != dimensional space)");
  }

  // Read the data values
  std::vector<int> m_elements; /**< raw values in sweep */

  m_elements.resize(m_dataElementCount);
  if (!hdf5Dataset.getValues(m_elements)) {
    throw std::runtime_error("Issue getting dataset values for 'data'");
  }
  // --------------------------------------------------------------------


  auto n = RadialSet::Create(
    m_name,                   // const std::string& TypeName,
    "dbZ",                    // const std::string& Units,
    location,                 // const LLH        & center,
    time,                     // const Time       & datatime,
    m_elevationDegs,          // const float      elevationDegrees,
    m_gate1RangeKMs * 1000.0, // const float      firstGateDistanceMeters,
    m_gateWidthMeters,        // const float      gateWidthMeters,
    m_rayCount,               // const size_t     num_radials,
    m_binCount);              // const size_t     num_gates);

  // ODIM_H5 standards dictate the /where/a1gate attribute is the
  // index (zero-based) of the first azimuth angle radiated in the scan.
  //
  // Canada appears to populate the /where/a1gate attribute as the
  // number of rays offset
  // offset of the number of rays in the scan versus the 1st ray radiated.
  //
  // Working example:
  // Canada reported /where/a1gate  = 226
  // Number of rays in scan         = 360 number of rays in the scan
  // Actual 1st index radiated      = 134 (#rays - a1gate)
  //
  // Actual example from an ODIM_H5 file:
  // Index[0]   = start time of 1569509765.0 epoch secs
  // ...
  // Index[133] = start time of 1569509769.0 epoch secs
  // Index[134] = start time of 1569509758.0 epoch secs <<-- actual 1st index radiated
  // ...
  // Index[226] = start time of 1569509760.0 epoch secs <<-- a1gate index reported
  // ...
  // Index[359] = start time of 1569509765.0 epoch secs
  //
  // Therefore, in order to obtain the proper temporal starting location of the
  // first ray radiated in the scan: ray count - a1gate.
  // -------------------------------------------------------------------------
  size_t m_firstRayRadiated; /**< 1st Ray Radiated in Sweep */

  m_firstRayRadiated = m_rayCount - m_a1gate;

  // -------------------------------------------------------------------------
  // First, do the 1D arrays in the RadialSet
  //
  auto& values = n->getFloat2DRef();
  auto& bw     = n->getFloat1DRef(RadialSet::BeamWidth);
  auto& gw     = n->getFloat1DRef(RadialSet::GateWidth);

  // We don't have special azimuth spacing by default, it's usually
  // the beamwidth.  But we can make it.
  auto& azsp = n->addFloat1DRef(RadialSet::AzimuthSpacing, "Degrees", { 0 });

  // Due to memory caching might be better as separate loops
  auto& az       = n->getFloat1DRef(RadialSet::Azimuth);
  size_t ri      = m_a1gate;
  size_t clipped = 0;
  auto * sa      = &m_azimuthStartAngles[0]; // alias

  for (size_t r = 0; r < m_rayCount; ++r) {
    // Set Azimuth
    az[r] = m_azimuthStartAngles[ri];

    // Set Beamwidth
    bw[r] = beamWidth;

    // Set AzimuthSpacing
    // Can't use az[r] since it's not fully set yet, need m_azimuthStartAngles
    double spacing = (ri == m_rayCount - 1) ? (360.0 + sa[0]) - sa[ri] : sa[ri + 1] - sa[ri];
    if (spacing < 0) { spacing = -spacing; }
    if (spacing > 1.5) { spacing = 1.0; clipped++; } // Max of 1.5 degrees
    azsp[r] = spacing;

    // Rolling
    if (++ri >= m_azimuthStartAngles.size()) {
      ri = 0;
    }
  }
  if (clipped > 0) {
    LogSevere("Dampened " << clipped << " AzmuthSpacings values to 1 degree\n");
  }

  // -------------------------------------------------------------------------
  // Second, do the 2D data array in the RadialSet, expanding values
  //
  // Since we're reading from data into RadialSet, we can just offset
  // by the number of a1gates (with data wrapping)
  size_t index = m_a1gate * m_binCount;
  // Avoid std::fabs calls and precompute bounds
  const float uLowerBound = m_nodataValue - 0.005f;
  const float uUpperBound = m_nodataValue + 0.005f;
  const float mLowerBound = m_undetectValue - 0.005f;
  const float mUpperBound = m_undetectValue + 0.005f;

  for (size_t r = 0; r < m_rayCount; ++r) {
    for (size_t g = 0; g < m_binCount; ++g) {
      const auto& vi = m_elements[index];
      auto& vo       = values[r][g];

      // Unavailable, Missing, or unpacked value
      if ((vi > uLowerBound) && (vi < uUpperBound)) {
        vo = Constants::DataUnavailable;
      } else if ((vi > mLowerBound) && (vi < mUpperBound)) {
        vo = Constants::MissingData;
      } else {
        vo = (m_gainCoeff * (vi * 1.0)) + m_offsetCoeff;
      }

      if (++index >= m_elements.size()) { // Roll
        index = 0;
      }
    }
  }

  // Attributes
  // FIXME: setRadarName, right?
  auto info = ConfigODIMProductInfo::getProductInfo(m_name);

  n->setRadarName(sourceName);
  n->setTypeName(info.getMRMSDataType());
  n->setUnits(info.getUnits());
  n->setDataAttributeValue("ColorMap", info.getColorMap());

  n->setDataAttributeValue("Nyquist_Vel", std::to_string(m_nyquistVelocity), "MetersPerSecond");
  n->setDataAttributeValue("ODIM_H5_malfunc", std::to_string(isMalfunction), "dimensionless");
  n->setDataAttributeValue("ODIM_H5_radar_msg", radarMsg, "dimensionless"); // Only if ! ""

  // n->setVCP() Needed or not.  'Kinda wana leave out VCP since things are supposed to not use it.

  output->addDataType(n);
  return true;
} // ODIMDataHandler::readODIM_MOMENT

bool
ODIMDataHandler::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys)
{
  // FIXME: Should be pretty easy to say write a single RadialSet as a SCAN,
  // or other types.  Don't know if we have a use case currently though
  // Might do it for fun.
  return false;
} // ODIMDataHandler::write
