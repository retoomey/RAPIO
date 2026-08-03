#pragma once

#include "rIOHDF5.h"
#include <rHDF5Specializer.h>

#include <rHDF5Group.h>
#include <rMultiDataType.h>

#include <hdf5.h>

namespace rapio {
/** Handles ODIM data
 *
 * @author Michael Taylor
 *    Original design
 *
 * @auther Robert Toomey
 *    Made it a IOSpecializer for ODIM data
 */
class ODIMDataHandler : public HDF5Specializer {
public:

  /** Destroy the specializer */
  virtual
  ~ODIMDataHandler();

  /** Initial introduction of ODIMDataHandler specializer to IOHDF5 */
  static void
  introduceSelf(IOHDF5 * owner);

  /** Read a DataType from HDF5 */
  virtual std::shared_ptr<DataType>
  readHDF5(hid_t hdf5id,
    IOConfig     & keys) override;

  /** Write DataType to HDF5 */
  virtual bool
  writeHDF5(hid_t             hdf5id,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;

protected:

  /** Read a ODIM SCANPVOL */
  std::shared_ptr<DataType>
  readODIM_SCANPVOL(hid_t hdf5id, double beamWidth, LLH location,
    const std::string& sourceName);

  /** Read a ODIM SCANPVOL_DATASET */
  bool
  readODIM_SCANPVOL_DATASET(
    std::shared_ptr<MultiDataType>& output,
    hdflib::HDF5Group& dataset,
    double beamWidth, LLH location,
    const std::string& sourceName);

  /** Read a ODIM Moment */
  bool
  readODIM_MOMENT(
    std::shared_ptr<MultiDataType>& output,
    hdflib::HDF5Group& datasetGroup,
    double beamWidth, LLH location, Time time,
    const std::string& sourceName, double m_elevationDegs,
    double m_gate1RangeKMs, double m_gateWidthMeters, size_t m_rayCount,
    size_t m_binCount, size_t m_a1gate, std::vector<double>& m_azimuthStartAngles,
    double m_nyquistVelocity, const std::string& radarMsg, bool isMalfunction);
};
}
