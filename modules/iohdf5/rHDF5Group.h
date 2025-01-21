#pragma once

#include "rHDF5Attribute.h"

/**
 * @brief Access HDF5 group objects
 *
 * Access HDF5 Groups from the HDF Group using its C-library interface.
 * @see https://www.hdfgroup.org/
 * @see https://portal.hdfgroup.org/display/HDF5/Groups
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html
 *
 * @author Michael C. Taylor (OU-CIMMS/NSSL)
 * @author Robert Toomeye
 *    Enhance to add Attribute support for cleaner API
 * @date   2020-01-06
 *
 *****************************************************************************/

#include <hdf5.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <rError.h>

namespace hdflib
{
// ****************************************************************************
//
// G R O U P    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class to access the HDF5 Group data interface
 *
 * Uses the HDF5 Group's C-library interface in connecting to HDF5
 * groups.
 * @see https://portal.hdfgroup.org/display/HDF5/Groups
 *
 * This class only throws an Exception when constructor is used to open Group.
 * Otherwise, no throws are performed in the class methods.
 *****************************************************************************/
class HDF5Group
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
   * hdflib::HDF5Group hdf5Group;
   * group.open(fileId, "what");
   * @endcode
   *************************************************************************/
  HDF5Group();

  /**
   * @brief Constructor to open group by name based on HDF5 file id.
   *
   * @note HDF5 resources are automatically cleaned up via default
   * destructor and RAII.
   *
   * @warning Constructor can throw the following:
   * @throws std::runtime_error exception if failure on open of group occurs.
   *
   * @param[in] t_id     HDF5 id of the opened file
   * @param[in] t_name   name of group to open for access
   * @param[in] t_aplId  group access property list (default to H5P_DEFAULT)
   *************************************************************************/
  HDF5Group(const hid_t t_id,
    const std::string   & t_name,
    const hid_t         t_aplId = H5P_DEFAULT);

  /** Does a group exist AT THIS LEVEL with given name?
   * Note: Don't pass in '/' paths */
  static bool
  hasGroup(hid_t t_id, const std::string& name);

  /** Do we have a given subgroup? */
  bool
  hasSubGroup(const std::string& name);

  /** Create a subgroup using our id */
  HDF5Group
  getSubGroup(const std::string& name);

  /** Get an attribute from the group */
  template <typename T>
  void
  getAttribute(const std::string& name, T * t_value) const
  {
    if (H5Aexists(m_id, name.c_str()) > 0) {
      HDF5Attribute attribute(m_id, name);
      attribute.getValue<T>(t_value); // Ensure this handles std::string correctly
      return;
    } else {
      LogInfo("Attribute '" << name << "' doesn't exist, will use default value of " << *t_value << "\n");
    }
    // throw std::runtime_error("Attribute read "+name+" failed");
  }

  /** Overload the std::string getAttribute method */
  void
  getAttribute(const std::string& name, std::string& t_value) const;

  /** Overload the size_t method since we use long with HDF5 to ensure size */
  void
  getAttribute(const std::string& name, size_t * t_value) const;

  // ------------------------------------------------------------------------
  // Destructors
  // ------------------------------------------------------------------------

  /**
   * @brief Default destructor.
   *
   * Closes group (if open) and releases outstanding HDF5 resources.
   * No Exception thrown using this Destructor
   *************************************************************************/
  virtual
  ~HDF5Group();

  /**
   * @brief Open HDF5 group using HDF5 file id and name of group.
   *
   * Opens group by name using HDF5 C-library H5Gopen2.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-Open2
   *
   * @note Default destructor automatically handles closing group to prevent
   * resource leaks.
   *
   * No Exception thrown using this method.
   *
   * @param[in] t_id     HDF5 id of the opened file
   * @param[in] t_name   name of group to open for access
   * @return  true if open successful; otherwise false
   *************************************************************************/
  bool
  open(const hid_t t_id, const std::string& t_name);

  /**
   * @brief Close opened HDF5 group and release resources.
   *
   * Closes group (if open) using HDF5 C-library H5Gclose.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-Close
   *
   * No Exception thrown using this method.
   *
   * @return  true if close successful; otherwise false
   *************************************************************************/
  bool
  close();

  /**
   * @brief Get HDF5 identifier of opened group.
   *
   * No Exception thrown using this method
   *
   * @return  opened group HDF5 id
   *************************************************************************/
  hid_t
  getId() const;

  /**
   * @brief Get number of objects that exists in opened group.
   *
   * Gets group information using HDF5 C-library H5Gget_info.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-GetInfo
   *
   * No Exception thrown using this method.
   *
   * @param[out] t_objectCount  populates with number of objects in opened group
   * @return  true if object count obtained; otherwise false
   *************************************************************************/
  bool
  getObjectCount(size_t& t_objectCount) const;

private:
  // ------------------------------------------------------------------------
  // Private member variables
  // ------------------------------------------------------------------------
  /** the HDF5 group identifier */
  hid_t m_id;

  /** the HDF5 group name */
  std::string m_name;

  /** the HDF5 group access property list - defaulted to H5P_DEFAULT */
  hid_t m_aplId;
}; // end of HDF5Group
} // end of namespace hdflib
