// Local Libraries
#include "rHDF5DataSpace.h"

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries
#include <string>
#include <iostream>
#include <stdexcept>

namespace hdflib
{
// ****************************************************************************
//
// B A S E    D A T A S P A C E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ---------------------------------------------------------------------------
// Default constructor for Base Class -- need to override depending on
// accessing Attribute versus Dataset dataspaces.
// ---------------------------------------------------------------------------
HDF5DataSpaceBase::HDF5DataSpaceBase()
  : m_id(-1)
{ } // end of HDF5DataSpaceBase::HDF5DataSpaceBase()

// ----------------------------------------------------------------------------
// Destructor for Base Class
// ----------------------------------------------------------------------------
HDF5DataSpaceBase::~HDF5DataSpaceBase()
{
  close();
} // end of HDF5DataSpaceBase::~HDF5DataSpaceBase()

// ----------------------------------------------------------------------------
// Close HDF5DataSpace and release outstanding HDF5 resources
// ----------------------------------------------------------------------------
bool
HDF5DataSpaceBase::close()
{
  bool isClosed = true;

  if (m_id >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened dataspace.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#DataSpace-Close
    //
    // Returns:
    // > 0: success and dataspace is closed
    // = 0: success and dataspace is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Sclose(m_id);
    if (status < 0) {
      isClosed = false;
    }
    m_id = -1;
  }
  return isClosed;
} // end of bool HDF5DataSpaceBase::close()

hid_t
HDF5DataSpaceBase::getId() const
{
  return m_id;
} // end of hid_t HDF5DataSpaceBase::getId() const

bool
HDF5DataSpaceBase::isSimple() const
{
  // -----------------------------------------------------------------------
  // Whether simple dataspace. Returns:
  //   > 0: success and is true
  //   = 0: success and is false (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  const htri_t status = H5Sis_simple(m_id);

  if (status <= 0) {
    return false;
  }

  return true;
} // end of bool HDF5DataSpaceBase::isSimple()

size_t
HDF5DataSpaceBase::getElementCount() const
{
  // -----------------------------------------------------------------------
  // Number of elements in dataspace. Returns:
  //   > 0: success and is number of elements in dataspace
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  const hssize_t elementCount = H5Sget_simple_extent_npoints(m_id);

  if (elementCount < 0) {
    throw std::runtime_error("H5Sget_simple_extent_npoints failed in HDF5DataSpaceBase::getElementCount");
  }

  return static_cast<size_t>(elementCount);
} // end of size_t HDF5DataSpaceBase::getElementCount() const

size_t
HDF5DataSpaceBase::getRank() const
{
  // -----------------------------------------------------------------------
  // Rank or number of dimensions in dataspace. Returns:
  //   > 0: success and is number of dimensions in dataspace
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  int rank = H5Sget_simple_extent_ndims(m_id);

  if (rank < 0) {
    throw std::runtime_error("H5Sget_simple_exent_ndims failed in HDF5DataSpaceBase::getRank");
  }
  return static_cast<size_t>(rank);
} // end of size_t HDF5DataSpaceBase::getRank() const

const std::vector<hsize_t>
HDF5DataSpaceBase::getDimensions(const size_t t_rank) const
{
  std::vector<hsize_t> dimensions(t_rank, 0);
  // -----------------------------------------------------------------------
  // Number of dimensions and maximum size of dataspace. Returns:
  //   > 0: success and is status of call
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  int status = H5Sget_simple_extent_dims(m_id, &dimensions.front(), NULL);

  if (status < 0) {
    throw std::runtime_error("H5Sget_simple_exent_dims failed in HDF5DataSpaceBase::getDimensions");
  }

  return dimensions;
} // end of std::vector<hsize_t> HDF5DataSpaceBase::getDimensions(const size_t t_rank) const

void
HDF5DataSpaceBase::setId(const hid_t t_id)
{
  m_id = t_id;
} // end of void HDF5DataSpaceBase::setId(const hid_t t_id)

// ****************************************************************************
//
// A T T R I B U T E    D A T A S P A C E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Constructor for HDF5 Attribute-based DataSpace (derived from HDF5DataSpaceBase)
// ----------------------------------------------------------------------------
HDF5AttributeSpace::HDF5AttributeSpace(const hid_t t_id)
{
  // -----------------------------------------------------------------------
  // Copy of attribute dataspace (H5Sclose to prevent leaks). Returns:
  //   > 0: success and is dataspace identifier
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  hid_t spaceId = H5Aget_space(t_id);

  if (spaceId < 0) {
    throw std::runtime_error("HDF5AttributeSpace failure in constructor - failed opening dataspace");
  }
  setId(spaceId);

  // Confirm we are working with a simple dataspace
  if (!isSimple()) {
    close();
    throw std::runtime_error("HDF5AttributeSpace failure in constructor - not a simple dataspace");
  }
} // end of HDF5AttributeSpace::HDF5AttributeSpace(const hid_t t_id)

// ****************************************************************************
//
// D A T A S E T    D A T A S P A C E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Constructor for HDF5 Dataset-based HDF5DataSpaces (derived from HDF5DataSpaceBase)
// ----------------------------------------------------------------------------
HDF5DatasetSpace::HDF5DatasetSpace(const hid_t t_id)
{
  // -----------------------------------------------------------------------
  // Copy of dataset dataspace (H5Sclose to prevent leaks). Returns:
  //   > 0: success and is dataspace identifier
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  hid_t spaceId = H5Dget_space(t_id);

  if (spaceId < 0) {
    throw std::runtime_error("HDF5DatasetSpace failure in constructor - failed opening dataspace");
  }
  setId(spaceId);

  // Confirm we are working with a simple dataspace
  if (!isSimple()) {
    close();
    throw std::runtime_error("HDF5DatasetSpace failure in constructor - not a simple dataspace");
  }
} // end of HDF5DatasetSpace::HDF5DatasetSpace(const hid_t t_id)
} // namespace hdflib
