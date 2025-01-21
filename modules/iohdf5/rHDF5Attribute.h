#pragma once

/**
 * @brief Access HDF5 Attribute interface from the HDF Group.
 *
 * Access HDF5 Attributes from the HDF Group using its C-library interface.
 * @see https://www.hdfgroup.org/
 * @see https://portal.hdfgroup.org/display/HDF5/Attributes
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html
 *
 * @author Michael C. Taylor (OU-CIMMS/NSSL)
 * @date   2020-01-06
 *****************************************************************************/

#include "rHDF5DataType.h"

#include <hdf5.h>

// System Libraries
#include <string>
#include <vector>
#include <stdexcept>

namespace hdflib
{
// ****************************************************************************
//
// P U B L I C    A T T R I B U T E    F U N C T I O N S
//
// ****************************************************************************

/**
 * @brief Check if HDF5 string-type Attribute exists under parent node.
 *
 * Determines if attribute exists using HDF5 C-library H5Aexists.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Exists
 *
 * No Exception thrown using this method
 *
 * @param[in] t_id    HDF5 group identifier
 * @param[in] t_name  name of attribute to verify exists
 * @return boolean true if attribute exists; otherwise false
 *****************************************************************************/
bool
isHDF5AttrExists(const hid_t t_id, const std::string& t_name);

/**
 * @brief Populate value from opened HDF5 scalar-type attribute.
 *
 * Gets value from HDF5 attribute using HDF5 C-library H5Aget.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
 *
 * No Exception thrown using this method
 *
 * @param[in] t_id      HDF5 group identifier
 * @param[in] t_name    name of attribute
 * @param[out] t_value  scalar-type value to populate from attribute
 * @return boolean true if populated successfully; otherwise false
 *****************************************************************************/
template <typename T>
bool
getAttribute(const hid_t t_id, const std::string& t_name, T * t_value);

/**
 * @brief Populate value from opened HDF5 string-type attribute.
 *
 * Gets value from HDF5 attribute using HDF5 C-library H5Aget.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
 *
 * No Exception thrown using this method
 *
 * @param[in] t_id      HDF5 group identifier
 * @param[in] t_name    name of attribute
 * @param[out] t_value  string-type value to populate from attribute
 * @return boolean true if populated successfully; otherwise false
 *****************************************************************************/
bool
getAttribute(const hid_t t_id, const std::string& t_name, std::string& t_value);

/**
 * @brief Populate vector from opened HDF5 array of scalar-type attribute values.
 *
 * Gets values from HDF5 array attribute using HDF5 C-library H5Aget.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
 *
 * No Exception thrown using this method
 *
 * @param[in] t_id      HDF5 group identifier
 * @param[in] t_name    name of array attribute
 * @param[out] t_values vector of scalars to populate from array attribute
 * @return boolean true if populated successfully; otherwise false
 *****************************************************************************/
bool
getAttributeValues(const hid_t t_id,
  const std::string            & t_name,
  std::vector<double>          & t_values);

// ****************************************************************************
//
// A T T R I B U T E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class used for obtaining HDF5 Attribute metadata
 *
 * Uses the HDF5 Attribute's C-library interface in connecting to HDF5 attributes.
 * @see https://portal.hdfgroup.org/display/HDF5/Attributes
 *
 * This class utilizes the std::runtime_error Exception in many of the its methods.
 *****************************************************************************/
class HDF5Attribute
{
public:

  /**
   * @brief Default constructor
   *
   * Constructor requires subsequent call to open method to use instance
   * properly. No Exception thrown using this constructor.
   *
   * Example:
   * @code
   * hdflib::HDF5Attribute hdf5Attr;
   * hdf5Attr.open(groupId, "lat");
   * @endcode
   ************************************************************************/
  HDF5Attribute();

  /**
   * @brief Constructor used to access HDF5 attribute by name
   *
   * Constructor opens the attribute by name using HDF5 C-library H5Aopen.
   *
   * @note Default destructor handles closing of the attribute to prevent
   * resource leaks.
   *
   * @warning Constructor can throw the following:
   * @throw std::runtime_error exception if failure on open occurs
   *
   * @param[in] t_id - HDF5 id of the opened file or group
   * @param[in] t_name - name of the attribute to open for access
   * @param[in] t_aplId - attribute access property list (default to H5P_DEFAULT)
   ************************************************************************/
  HDF5Attribute(const hid_t t_id,
    const std::string       & t_name,
    const hid_t             t_aplId = H5P_DEFAULT);

  /**
   * @brief Default destructor
   *
   * Closes attribute (if open) and releases outstanding HDF5 resources.
   * No Exception thrown using this Destructor
   *************************************************************************/
  virtual
  ~HDF5Attribute();

  // ------------------------------------------------------------------------
  // Public Methods
  // ------------------------------------------------------------------------

  /**
   * @brief Open HDF5 attribute using HDF5 file or group id and name of attribute.
   *
   * Opens attribute by name using HDF5 C-library H5Aopen.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Open
   *
   * No Exception thrown using this method
   *
   * @param[in] t_id    HDF5 identifier of file or group
   * @param[in] t_name  name of attribute to open for access
   * @param[in] t_aplId attribute access property list (default to H5P_DEFAULT)
   * @return true if close successful; otherwise false
   *************************************************************************/
  bool
  open(const hid_t   t_id,
    const std::string& t_name,
    const hid_t      t_aplId = H5P_DEFAULT);

  /**
   * @brief Close the opened HDF5 attribute and release resources.
   *
   * Closes attribute (if open) using HDF5 C-library H5Aclose.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Close
   *
   * No Exception thrown using this method
   *
   * @return true if close successful; otherwise false
   *************************************************************************/
  bool
  close();

  /**
   * @brief Get attribute id of opened attribute.
   *
   * No Exception thrown using this method
   *
   * @return opened attribute HDF5 id
   *************************************************************************/
  hid_t
  getId() const;

  /**
   * @brief Read scalar-type value from opened HDF5 attribute
   *
   * Get Attribute values of scalar-type such as long, double, etc.
   *
   * Get value from HDF5 attribute using HDF5 C-library H5Aget.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
   *
   * @throw std::runtime_error exception if failure occurs.
   *
   * @param[out] t_value   scalar-type value to populate with attribute
   * @return boolean true if successful read; otherwise false
   *************************************************************************/
  template <typename T>
  void
  getValue(T * t_value) const
  {
    HDF5AttrType attrType(m_id);

    // -------------------------------------------------------------------------
    // Read the attribute data value from the file into memory
    // allowing HDF5 to perform conversion on the fly.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
    //
    // Returns:
    // > 0: success
    // = 0: success (confirmed by HDF)
    // < 0: failure
    // -------------------------------------------------------------------------
    const herr_t status = H5Aread(m_id, attrType.getNativeId(), t_value);

    if (status < 0) {
      throw std::runtime_error("Failure reading Scalar Attribute");
    }
  } // end of bool HDF5Attribute::getValue(double* t_value)

  /**
   * @brief Get string-type value from opened HDF5 attribute
   *
   * Method will read fixed-length or variable-length strings.
   * @note variable-length strings require further to ensure logic
   * is correctly reading values and cleaning up resources.
   *
   * Get value from HDF5 attribute using HDF5 C-library H5Aget.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
   *
   * @throw std::runtime_error exception if failure occurs.
   *
   * @param[out] t_value  string-type value to populate with attribute
   * @return boolean true if successful read; otherwise false
   *************************************************************************/
  void
  getValue(std::string& t_value) const;

  /**
   * @brief Get array of scalar-type values from opened HDF5 array attribute
   *
   * Get scalar-type values from attribute array such as double using
   * HDF5 C-library.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
   *
   * @throw std::runtime_error exception if failure occurs.
   *
   * @param[out] t_values vector of scalars to populate with array attributes
   * @return boolean true if successful read; otherwise false
   *************************************************************************/
  template <typename T>
  void
  getValues(std::vector<T>& t_values) const;

private:

  /**
   * @brief Get variable-length string from opened HDF5 attribute
   *
   * @throw std::runtime_error exception if failure occurs.
   *
   * @param[in] t_attrType  instance of attribute's file-based datatype
   * @param[out] t_value    string populated with attribute value
   *************************************************************************/
  void
  getVariableString(const HDF5AttrType& t_attrType,
    std::string                       & t_value) const;

  /**
   * @brief Get fixed-length string from opened HDF5 attribute
   *
   * @throw std::runtime_error exception if failure occurs.
   *
   * @param[in] t_attrType  instance of attribute's file-based datatype
   * @param[out] t_value    string populated with attribute value
   *************************************************************************/
  void
  getFixedString(const HDF5AttrType& t_attrType,
    std::string                    & t_value) const;

  /** the attribute HDF5 identifier */
  hid_t m_id;

  /** the attribute name */
  std::string m_name;
}; // end of HDF5Attribute
} // end of namespace hdflib
