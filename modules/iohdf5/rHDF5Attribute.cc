// Local Libraries
#include "rHDF5Attribute.h"
#include "rHDF5DataType.h"
#include "rHDF5DataSpace.h"

// RAPIO Libraries
#include <rError.h>

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries
#include <stdlib.h> // free
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace hdflib
{
// ****************************************************************************
//
// P U B L I C    A T T R I B U T E    F U N C T I O N S
//
// ****************************************************************************

// ---------------------------------------------------------------------------
// Returns true if attribute exists immediately under parent node.
// ---------------------------------------------------------------------------
bool
isHDF5AttrExists(const hid_t t_id, const std::string& t_name)
{
  // -------------------------------------------------------------------------
  // Returns whether HDF5 Attribute exists based on attribute name passed.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Exists
  //   > 0: attribute exists
  //   = 0: attribute does not exist
  //   < 0: function failed or attribute does not exist
  // -------------------------------------------------------------------------
  return H5Aexists(t_id, t_name.c_str()) > 0;
} // end of bool isHDF5AttrExists(const hid_t t_id, const std::string& t_name)

// ---------------------------------------------------------------------------
// Convenience method to read an attribute value of scalar-type by name.
// ---------------------------------------------------------------------------
template <typename T>
bool
getAttribute(const hid_t t_id, const std::string& t_name, T * t_value)
{
  if (!isHDF5AttrExists(t_id, t_name)) {
    return false;
  }

  try
  {
    HDF5Attribute attribute(t_id, t_name);
    attribute.getValue(t_value);
  }
  catch (const std::runtime_error& re)
  {
    fLogSevere("WARN: Failure getting value for Scalar Attribute: {}. Error: {}", t_name, re.what());
    return false;
  }

  return true;
} // end of bool getAttribute(const hid_t t_id, const std::string& t_name, T* t_value)

// ---------------------------------------------------------------------------
// Explicitly instantiate function templates
// ---------------------------------------------------------------------------
template bool
getAttribute(const hid_t t_id, const std::string& t_name, double * t_value);
template bool
getAttribute(const hid_t t_id, const std::string& t_name, long * t_value);

// ---------------------------------------------------------------------------
// Convenience method to read an attribute of string-type by name.
// ---------------------------------------------------------------------------
bool
getAttribute(const hid_t t_id, const std::string& t_name, std::string& t_value)
{
  if (!isHDF5AttrExists(t_id, t_name)) {
    return false;
  }
  try
  {
    HDF5Attribute attribute(t_id, t_name);
    attribute.getValue(t_value);
  }
  catch (const std::runtime_error& re)
  {
    fLogSevere("WARN: Failure getting value for String Attribute: {}. Error: {}", t_name, re.what());
    return false;
  }

  return true;
} // end of bool getAttribute(const hid_t t_id, const std::string& t_name, std::string& t_value)

// ---------------------------------------------------------------------------
// Convenience method to read an array of attributes of double-type by name.
// ---------------------------------------------------------------------------
bool
getAttributeValues(const hid_t t_id,
  const std::string            & t_name,
  std::vector<double>          & t_values)
{
  // Verify array attribute exists before open/read attempt
  if (!isHDF5AttrExists(t_id, t_name)) {
    return false;
  }
  try
  {
    HDF5Attribute attribute(t_id, t_name);
    attribute.getValues(t_values);
  }
  catch (const std::runtime_error& re)
  {
    fLogSevere("WARN: Failure getting values for Array Attribute: {}. Error: {}", t_name, re.what());
    return false;
  }

  return true;
} // end of bool getAttributeValues(const hid_t t_id,...

// ****************************************************************************
//
// A T T R I B U T E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
HDF5Attribute::HDF5Attribute()
  : m_id(-1), m_name("")
{ } // end of HDF5Attribute::HDF5Attribute()

// ----------------------------------------------------------------------------
// Constructor - automatically opens HDF5 attribute for use
// ----------------------------------------------------------------------------
HDF5Attribute::HDF5Attribute(const hid_t t_id,
  const std::string                      & t_name,
  const hid_t                            t_aplId)
  : m_id(-1), m_name(t_name)
{
  if (!open(t_id, t_name, t_aplId)) {
    throw std::runtime_error("HDF5Attribute failure in constructor - failed opening attribute");
  }
} // end of HDF5Attribute::HDF5Attribute(const hid_t t_id,

// ----------------------------------------------------------------------------
// Destructor - closes attribute (if open) and releases HDF5 resources
// ----------------------------------------------------------------------------
HDF5Attribute::~HDF5Attribute()
{
  close();
} // end of HDF5Attribute::~HDF5Attribute()

// ----------------------------------------------------------------------------
// Open HDF5Attribute for access
// ----------------------------------------------------------------------------
bool
HDF5Attribute::open(const hid_t t_id,
  const std::string             & t_name,
  const hid_t                   t_aplId)
{
  m_name = t_name;
  // -----------------------------------------------------------------------
  // HDF5 C-lib function to open existing attribute by name
  // Must issue H5Aclose after use to prevent resource leaks
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Open
  // > 0: success and is attribute identifier
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -----------------------------------------------------------------------
  m_id = H5Aopen(t_id, t_name.c_str(), t_aplId);
  if (m_id < 0) {
    return false;
  }
  return true;
} // end of bool HDF5Attribute::open(const hid_t t_id,

// ----------------------------------------------------------------------------
// Close HDF5Attribute and release outstanding HDF5 resources
// ----------------------------------------------------------------------------
bool
HDF5Attribute::close()
{
  bool isClosed = true;

  if (m_id >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened attribute.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Close
    //
    // Returns:
    // > 0: success and attribute is closed
    // = 0: success and attribute is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Aclose(m_id);
    if (status < 0) {
      isClosed = false;
    }
    m_id = -1;
  }
  return isClosed;
} // end of bool HDF5Attribute::close()

// ----------------------------------------------------------------------------
// Return opened attribute identifier
// ----------------------------------------------------------------------------
hid_t
HDF5Attribute::getId() const
{
  return m_id;
} // end of hid_t HDF5Attribute::getId() const

#if 0
// ----------------------------------------------------------------------------
// Template method to get Attribute values of Scalar type (long, double, etc.)
// ----------------------------------------------------------------------------
template <typename T>
void
HDF5Attribute::getValue(T * t_value) const
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

#endif // if 0

// ---------------------------------------------------------------------------
// Explicitly instantiate method templates
// ---------------------------------------------------------------------------
// FIXME: These appear to be unnecessary for the code to work. I'm not sure why.
// If these are commented out the code still compiles and creates data output
// correctly. So, not sure why this works - but leaving in just in case and
// to account for what data types are expected.
// template void
// HDF5Attribute::getValue(double * t_value) const;
// template void
// HDF5Attribute::getValue(long * t_value) const;

// ----------------------------------------------------------------------------
// Get Attribute value of String type (either fixed or variable length)
// ----------------------------------------------------------------------------
void
HDF5Attribute::getValue(std::string& t_value) const
{
  // Verify file's datatype is HDF atomic string type. Enables HDF to safely
  // convert string-based data from file into string-compatible memory type.
  HDF5AttrType attrType(m_id);

  if (!attrType.isString()) {
    throw std::runtime_error("Invalid datatype class - expected string type");
  }

  // -------------------------------------------------------------------------
  // Determine if string is variable or fixed length.
  // -------------------------------------------------------------------------
  if (attrType.isVariableString()) {
    getVariableString(attrType, t_value);
  } else {
    getFixedString(attrType, t_value);
  }
} // end of void HDF5Attribute::getValue(std::string& t_value) const

// ----------------------------------------------------------------------------
// Get Attribute Array values
// ----------------------------------------------------------------------------
template <typename T>
void
HDF5Attribute::getValues(std::vector<T>& t_values) const
{
  // Ensure we are working with empty zero-sized vector before we begin
  std::vector<T>().swap(t_values);

  HDF5AttrType attrType(m_id);

  HDF5AttributeSpace attrSpace(m_id);

  // At this time - only supporting simple one-dimensional attribute array types.
  const size_t maxDimensionsAllowed(1);
  const size_t rank = attrSpace.getRank();

  if (rank != maxDimensionsAllowed) {
    throw std::runtime_error("Unsupported array type (more than one-dimension)");
  }

  // Size vector according to how many elements we expected to read.
  const size_t dataSize = attrSpace.getElementCount();

  t_values.reserve(dataSize);
  for (size_t idx = 0; idx < dataSize; ++idx) {
    t_values.push_back(0);
  }

  // -------------------------------------------------------------------------
  // HDF5 C Interface: Read attribute data value from the file into memory
  // allowing HDF5 to perform conversion on the fly.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
  // Returns:
  // > 0: success
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -------------------------------------------------------------------------
  const herr_t status = H5Aread(m_id, attrType.getNativeId(), &t_values.front());

  if (status < 0) {
    throw std::runtime_error("H5Aread failure for Attribute of scalar-type");
  }

  if (dataSize != t_values.size()) {
    throw std::runtime_error("Array Attribute Data size mismatch between expected and actual");
  }
} // end of void HDF5Attribute::getValues(std::vector<T>& t_values) const

// ---------------------------------------------------------------------------
// Explicitly instantiate method templates
// ---------------------------------------------------------------------------
template void
HDF5Attribute::getValues(std::vector<double>& t_values) const;
template void
HDF5Attribute::getValues(std::vector<float>& t_values) const;

// ---------------------------------------------------------------------------
// Get variable-length strings
// FIXME: Did not have examples to test with so at some point need
// to test to ensure there is not a memory leak when we return back
// from the H5Aread call. If there is - we may need to call H5Dvlen_reclaim.
// @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-VLReclaim
// ---------------------------------------------------------------------------
void
HDF5Attribute::getVariableString(const HDF5AttrType& t_attrType,
  std::string                                      & t_value) const
{
  fLogDebug("DEBUG: Processing Variable-length String attribute");

  char * rawValues;

  // -------------------------------------------------------------------------
  // Read the attribute data value from the file into memory
  // allowing HDF5 to perform conversion on the fly.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
  // Returns:
  // > 0: success
  // = 0: success (confirmed by HDF)
  // < 0: failure
  // -------------------------------------------------------------------------
  const herr_t status = H5Aread(m_id, t_attrType.getNativeId(), &rawValues);

  if (status < 0) {
    if (rawValues != NULL) { // Make sure actually have an address to free
      free(rawValues);
    }
    throw std::runtime_error("H5Aread failure for variable-length string attribute");
  }
  t_value = rawValues;
  free(rawValues);
} // void HDF5Attribute::getVariableString(const HDF5AttrType& t_attrType,...

// ----------------------------------------------------------------------------
// Get Attribute Fixed-length strings
// @note fixed-length strings require the size of the data to be calculated
// based on the number of elements in the file * the size each element will
// take up in memory (plus 1 for the null byte).
// FIXME: Started out trying to use vector<char> instead of char array
// but could not get the null byte working. The returning string always had
// an extra null byte or more.
// ----------------------------------------------------------------------------
void
HDF5Attribute::getFixedString(const HDF5AttrType& t_attrType,
  std::string                                   & t_value) const
{
  HDF5AttributeSpace attrSpace(m_id);

  const size_t dataSize = t_attrType.getNativeSize() * attrSpace.getElementCount();

  if (dataSize > 0) {
    char * rawValues = new char[dataSize + 1];
    // -------------------------------------------------------------------------
    // Read the attribute data value from the file into memory
    // allowing HDF5 to perform conversion on the fly.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-Read
    // Returns:
    // > 0: success
    // = 0: success (confirmed by HDF)
    // < 0: failure
    // -------------------------------------------------------------------------
    const herr_t readStatus = H5Aread(m_id, t_attrType.getNativeId(), rawValues);
    if (readStatus < 0) {
      delete []rawValues;
      throw std::runtime_error("H5Aread failure for fixed-length string attribute");
    }

    rawValues[dataSize] = '\0';
    t_value = rawValues;
    delete []rawValues;
  } // end of if (dataSize > 0)
}   // end of void HDF5Attribute::getFixedString(const HDF5AttrType& t_attrType,...
} // namespace hdflib
