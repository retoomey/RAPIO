#include "rHDF5Group.h"
#include "rError.h"

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
// G R O U P    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
HDF5Group::HDF5Group()
  : m_id(-1), m_name(""), m_aplId(H5P_DEFAULT)
{ } // end of HDF5Group::HDF5Group()

// ----------------------------------------------------------------------------
// Constructor - automatically opens HDF5 group for use
// ----------------------------------------------------------------------------
HDF5Group::HDF5Group(const hid_t t_id,
  const std::string              & t_name,
  const hid_t                    t_aplId)
  : m_id(-1), m_name(t_name), m_aplId(t_aplId)
{
  if (!open(t_id, t_name)) {
    throw std::runtime_error("HDF5Group failure in constructor - failed opening group");
  }
} // end of HDF5Group::HDF5Group constructor

bool
HDF5Group::hasGroup(const hid_t t_id, const std::string& name)
{
  // Check if the link exists
  if (H5Lexists(t_id, name.c_str(), H5P_DEFAULT) <= 0) {
    return false;
  }

  // Get object info
  H5O_info2_t obj_info;

  if (H5Oget_info_by_name3(t_id, name.c_str(), &obj_info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
    return false; // Unable to get object info
  }

  // Check if the object is a group
  return (obj_info.type == H5O_TYPE_GROUP);
}

bool
HDF5Group::hasSubGroup(const std::string& name)
{
  return hasGroup(m_id, name);
}

HDF5Group
HDF5Group::getSubGroup(const std::string& name)
{
  HDF5Group sub(m_id, name, H5P_DEFAULT);

  return sub;
}

// ----------------------------------------------------------------------------
// Destructor - closes group (if open) and release HDF5 resources
// ----------------------------------------------------------------------------
HDF5Group::~HDF5Group()
{
  close();
} // end of HDF5Group::~HDF5Group()

// ----------------------------------------------------------------------------
// Open HDF5Group for access
// ----------------------------------------------------------------------------
bool
HDF5Group::open(const hid_t t_id, const std::string& t_name)
{
  m_name = t_name;
  // -----------------------------------------------------------------------
  // HDF5 C Interface to open existing group by name. H5Gclose to
  // prevent resource leaks.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-Open2
  //
  // Returns:
  // > 0: success and is group identifier
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -----------------------------------------------------------------------
  m_id = H5Gopen2(t_id, t_name.c_str(), m_aplId);
  if (m_id < 0) {
    return false;
  }

  return true;
} // end of bool HDF5Group::open(const hid_t t_id, const std::string& t_name)

// ----------------------------------------------------------------------------
// Close HDF5Group and release outstanding HDF5 resources
// ----------------------------------------------------------------------------
bool
HDF5Group::close()
{
  bool isClosed = true;

  if (m_id >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened group.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-Close
    //
    // Returns:
    // > 0: success and group is closed
    // = 0: success and group is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Gclose(m_id);
    if (status < 0) {
      isClosed = false;
    }
    m_id = -1;
  }
  return isClosed;
} // end of bool HDF5Group::close()

void
HDF5Group::getAttribute(const std::string& name, std::string& t_value) const
{
  if (H5Aexists(m_id, name.c_str()) > 0) {
    HDF5Attribute attribute(m_id, name);
    attribute.getValue(t_value); // Ensure this handles std::string correctly
    return;
  }
  // throw std::runtime_error("Attribute read " + name + " failed");
}

void
HDF5Group::getAttribute(const std::string& name, size_t * t_value) const
{
  if (H5Aexists(m_id, name.c_str()) > 0) {
    HDF5Attribute attribute(m_id, name);
    long temp; // Ensure this handles size_t correctly.
    attribute.getValue(&temp);
    *t_value = static_cast<size_t>(temp);
    return;
  }
  // throw std::runtime_error("Attribute read " + name + " failed");
}

// ----------------------------------------------------------------------------
// Get HDF5 Group Id
// ----------------------------------------------------------------------------
hid_t
HDF5Group::getId() const
{
  return m_id;
} // end of hid_t HDF5Group::getId() const

// ----------------------------------------------------------------------------
// Get Number of Objects in HDF5Group
// ----------------------------------------------------------------------------
bool
HDF5Group::getObjectCount(size_t& t_objectCount) const
{
  H5G_info_t groupInfo;
  // -----------------------------------------------------------------------
  // HDF5 C Interface to obtain group information including number of links
  // in the group.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-GetInfo
  //
  // Returns:
  // > 0: success
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -----------------------------------------------------------------------
  herr_t status = H5Gget_info(m_id, &groupInfo);

  if (status < 0) {
    return false;
  }
  t_objectCount = static_cast<size_t>(groupInfo.nlinks);

  return true;
} // end of size_t HDF5Group::getObjectCount() const
} // namespace hdflib
