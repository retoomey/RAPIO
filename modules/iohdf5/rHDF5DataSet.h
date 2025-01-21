#pragma once

/**
 * @brief Access HDF5 dataset objects.
 *
 * Access HDF% Datasets from the HDF Group using its C-library interface.
 * @see https://www.hdfgroup.org/
 * @see https://portal.hdfgroup.org/display/HDF5/Datasets
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html
 *
 * @author Michael C. Taylor (OU-CIMMS/NSSL)
 * @date   2020-01-06
 *****************************************************************************/

// Local Libraries

// RAPIO Libraries

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries
#include <string>
#include <vector>

namespace hdflib
{
// ****************************************************************************
//
//  D A T A S E T    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class to access the HDF5 dataset interface.
 *
 * Uses the HDF5 Group's C-library interface in connecting to HDF5 datasets.
 * @see https://portal.hdfgroup.org/display/HDF5/Datasets
 *
 * This class only throws Exception when constructor is used to open dataset.
 * Otherwise, no throws are performed in the class methods.
 *****************************************************************************/
class HDF5DataSet
{
public:

  /**
   * @brief Default constructor
   *
   * Constructor requires subsequent call to open method to use instance
   * properly. No Exceptions thrown using this constructor.
   *
   * Example:
   * @code
   * hdflib::HDF5Dataset hdf5Dataset;
   * hdf5Dataset.open(groupId, 'data");
   * @endcode
   *************************************************************************/
  HDF5DataSet();

  /**
   * @brief Constructor to open dataset by name based on HDF5 identifier
   *
   * @note HDF5 resources are automatically cleaned up via default
   * destructor and RAII.
   *
   * @warning Constructor can throw the following:
   * @throw std::runtime_error if failure to open occurs
   *
   * @param[in] t_id - HDF5 identifier of the opened file or group
   * @param[in] t_name - name of the dataset to open for access
   * @param[in] t_aplId - dataset access property list (default to H5P_DEFAULT)
   *************************************************************************/
  HDF5DataSet(const hid_t t_id,
    const std::string     & t_name,
    const hid_t           t_aplId = H5P_DEFAULT);

  /**
   * @brief Default destructor
   *
   * Closes dataset (if open) and releases outstanding HDF5 resources.
   * No Exception thrown using this Destructor
   *************************************************************************/
  virtual
  ~HDF5DataSet();

  /**
   * @brief Open HDF5 dataset using HDF5 identifier and name of dataset.
   *
   * Opens dataset by name using HDF5 C-library H5Dopen2.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open2
   *
   * No Exception thrown using this method.
   *
   * @param[in] t_id - HDF5 identifier of the opened file or group
   * @param[in] t_name - name of the dataset to open for access
   * @param[in] t_aplId - dataset access property list (default to H5P_DEFAULT)
   * @return true if open successful; otherwise false.
   *************************************************************************/
  bool
  open(const hid_t   t_id,
    const std::string& t_name,
    const hid_t      t_aplId = H5P_DEFAULT);

  /**
   * @brief Closes opened HDF5 dataset and releases resources.
   *
   * Closes dataset (if open) using HDF5 C-library H5Dclose.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
   *
   * No Exception thrown using this method.
   *
   * @return true if open successful; otherwise false.
   *************************************************************************/
  bool
  close();

  /**
   * @brief Get HDF5 dataset identifier of opened dataset.
   *
   * No Exception thrown using this method.
   *
   * @return opened dataset HDF5 identifier
   *************************************************************************/
  hid_t
  getId() const;

  /**
   * @brief Get HDF5 dataset integer values from dataset array
   *
   * Reads dataset using HDF5 C-library H5Dread.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Read
   *
   * HDF5 will read the data from the file and convert its file-based datatype
   * into a native (memory) based datatype on the fly.
   *
   * No Exception thrown using this method.
   *
   * @param[out] t_values  vector of integers to populate from dataset array
   * @param[in] t_nativeDataType  memory datatype to use in loading into memory space
   *                              default=H5T_NATIVE_INT
   * @param[in] t_nativeDataSpace memory dataspace that data values are loaded into
   *                              default=H5S_ALL
   * @param[in] t_fileDataSpace   file-based dataspace data values are read from
   *                              default=H5S_ALL
   * @return boolean true if read of dataset values successful; otherwise false.
   *************************************************************************/
  bool
  getValues(std::vector<int>& t_values,
    const hid_t             t_nativeDataType  = H5T_NATIVE_INT,
    const hid_t             t_nativeDataSpace = H5S_ALL,
    const hid_t             t_fileDataSpace   = H5S_ALL);

private:
  /** the HDF5 dataset identifier */
  hid_t m_id;

  /** the HDF5 dataset name */
  std::string m_name;

  /** the HDF5 dataset access property list */
  hid_t m_aplId;
}; // end of HDF5DataSet
} // end of namespace hdflib
