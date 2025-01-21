// Local Libraries
#include "rHDF5DataSet.h"

// RAPIO Libraries

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace hdflib
{
// ****************************************************************************
//
// D A T A S E T    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ---------------------------------------------------------------------------
// Default constructor
// ---------------------------------------------------------------------------
HDF5DataSet::HDF5DataSet()
  : m_id(-1), m_name(""), m_aplId(H5P_DEFAULT)
{ } // end of HDF5DataSet::HDF5DataSet()

// ---------------------------------------------------------------------------
// Constructor - automatically opens HDF5 dataset for use
// ---------------------------------------------------------------------------
HDF5DataSet::HDF5DataSet(const hid_t t_id,
  const std::string                  & t_name,
  const hid_t                        t_aplId)
  : m_id(-1), m_name(t_name), m_aplId(t_aplId)
{
  if (!open(t_id, t_name, t_aplId)) {
    throw std::runtime_error("Error opening dataset in HDF5DataSet Constructor");
  }
} // end of HDF5DataSet::HDF5DataSet(const hid_t t_id,...

// ---------------------------------------------------------------------------
// Destructor - closes dataset (if open) and releases HDF5 resources
// ---------------------------------------------------------------------------
HDF5DataSet::~HDF5DataSet()
{
  close();
} // end of HDF5DataSet::~HDF5DataSet()

// ----------------------------------------------------------------------------
// Open DataSet for access
// ----------------------------------------------------------------------------
bool
HDF5DataSet::open(const hid_t t_id,
  const std::string           & t_name,
  const hid_t                 t_aplId)
{
  m_name  = t_name;
  m_aplId = t_aplId;
  // -----------------------------------------------------------------------
  // HDF5 C-lib function to open existing dataset by name. H5Dclose to
  // prevent resource leaks.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open2
  // Returns:
  // > 0: success and is dataset identifier
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -----------------------------------------------------------------------
  m_id = H5Dopen2(t_id, t_name.c_str(), t_aplId);
  if (m_id < 0) {
    return false;
  }
  return true;
} // end of bool HDF5DataSet::open(const hid_t t_id,...

// ----------------------------------------------------------------------------
// Close DataSet and release outstanding HDF5 resources
// ----------------------------------------------------------------------------
bool
HDF5DataSet::close()
{
  bool isClosed = true;

  if (m_id >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened dataset.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
    // Returns:
    // > 0: success and dataset is closed
    // = 0: success and dataset is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Dclose(m_id);
    if (status < 0) {
      isClosed = false;
    }
    m_id = -1;
  }
  return isClosed;
} // end of bool HDF5DataSet::close()

// ----------------------------------------------------------------------------
// Returned already opened HDF5 dataset identifier
// ----------------------------------------------------------------------------
hid_t
HDF5DataSet::getId() const
{
  return m_id;
} // end of hid_t HDF5DataSet::getId() const

// ----------------------------------------------------------------------------
// Read values from HDF5 dataset array of integers
// TODO: Future change should be to use a template instead of override.
// ----------------------------------------------------------------------------
bool
HDF5DataSet::getValues(std::vector<int>& t_values,
  const hid_t                          t_nativeDataType,
  const hid_t                          t_nativeDataSpace,
  const hid_t                          t_fileDataSpace)
{
  // -------------------------------------------------------------------------
  // Read dataset values from file into memory allowing HDF5 C-library to
  // perform conversion on the fly.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Read
  // Returns:
  // > 0: success and is dataset identifier
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -------------------------------------------------------------------------
  const herr_t status = H5Dread(m_id,
      t_nativeDataType,
      t_nativeDataSpace,
      t_fileDataSpace,
      m_aplId,
      &t_values.front());

  if (status < 0) {
    return false;
  }
  return true;
} // end of bool HDF5DataSet::getValues(std::vector<int>& t_values,
} // namespace hdflib
