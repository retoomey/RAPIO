#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rIO.h"

#include <hdf5.h>
#include <iomanip>

namespace rapio {
/**
 * HDF5 ability
 *
 * @author Michael Taylor, Robert Toomey
 *
 */
class IOHDF5 : public IODataType {
public:

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  virtual
  ~IOHDF5();

  // UTILITY ------------------------------------------------------------

  /** Determine whether file is in the HDF5 File Format */
  static bool
  isHdf5File(const std::string& t_filename);

  /**
   * @brief Determine if HDF5 object exists immediately under parent node.
   *
   * Useful for answering if a group exist within a given HDF5 file or if
   * an attribute exists within a given HDF5 group?
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5L.html#Link-Exists
   *
   * No Exception thrown using this function.
   *
   * @param[in] t_id    HDF5 identifier of opened file or group
   * @param[in] t_name  HDF5 object name to verify exists
   * @param[in] t_aplId object access property list (defaults to H5P_DEFAULT)
   * @return true if object exists otherwise false
   */
  static bool
  isObjectExists(const hid_t t_id,
    const std::string        & t_name,
    const hid_t              t_aplId = H5P_DEFAULT);
};
}
