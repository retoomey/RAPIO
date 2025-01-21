#include "rHDF5DataType.h"

#include <hdf5.h>

// System Libraries
#include <string>
#include <iostream>
#include <stdexcept>

namespace hdflib
{
// ****************************************************************************
//
// B A S E    D A T A T Y P E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Public Definitions
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Base Constructor for HDF5 Datatypes
// ----------------------------------------------------------------------------
HDF5DataTypeBase::HDF5DataTypeBase()
  : m_id(-1), m_nativeId(-1)
{
  // In your override of the constructor -- add following method calls:
  // try
  // {
  //  open(t_id);
  // }
  // catch(...)
  // {
  //  close();
  //  throw;
  // }
} // end of HDF5DataTypeBase::HDF5DataTypeBase()

// ----------------------------------------------------------------------------
// Base Destructor for HDF5 Datatypes
// ----------------------------------------------------------------------------
HDF5DataTypeBase::~HDF5DataTypeBase()
{
  close();
} // end of HDF5DataTypeBase::~HDF5DataTypeBase()

// ----------------------------------------------------------------------------
// Method to open an Attribute or Dataset file-based datatype and class as well
// as their native (memory) based datatype.
// ----------------------------------------------------------------------------
void
HDF5DataTypeBase::open(const hid_t t_id)
{
  openDataType(t_id);
  setClassType();
  openNativeType();
}

// ----------------------------------------------------------------------------
// Close Datatype and release outstanding HDF5 resources
// ----------------------------------------------------------------------------
bool
HDF5DataTypeBase::close()
{
  bool isClosed = true;

  if (m_id >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened file datatype.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-Close
    //
    // Returns:
    // > 0: success and datatype is closed
    // = 0: success and datatype is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Tclose(m_id);
    if (status < 0) {
      isClosed = false;
    }
    m_id = -1;
  }

  if (m_nativeId >= 0) {
    // -----------------------------------------------------------------------
    // HDF5 C Interface to close opened native datatype.
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-Close
    //
    // Returns:
    // > 0: success and file is closed
    // = 0: success and file is closed (confirmed by HDF)
    // < 0: failure
    // -----------------------------------------------------------------------
    const herr_t status = H5Tclose(m_nativeId);
    if (status < 0) {
      isClosed = false;
    }
    m_nativeId = -1;
  }

  return isClosed;
} // end of const bool HDF5DataTypeBase::close()

// ----------------------------------------------------------------------------
// Get identifier of file-based datatype
// ----------------------------------------------------------------------------
hid_t
HDF5DataTypeBase::getId() const
{
  return m_id;
} // end of const hid_t HDF5DataTypeBase::getId() const

// ----------------------------------------------------------------------------
// Get identifier of memory-based (native) datatype
// ----------------------------------------------------------------------------
hid_t
HDF5DataTypeBase::getNativeId() const
{
  return m_nativeId;
} // end of const hid_t HDF5DataTypeBase::getNativeId() const

// ----------------------------------------------------------------------------
// Get class type of file-based datatype
// ----------------------------------------------------------------------------
H5T_class_t
HDF5DataTypeBase::getClassType() const
{
  return m_classType;
} // end of const H5T_class_t HDF5DataTypeBase::getClassType() const

// ----------------------------------------------------------------------------
// Conveniance methods to determine if class type of file-based datatype.
// Useful in ensuring conversion from file to memory will be successful.
// ----------------------------------------------------------------------------
bool
HDF5DataTypeBase::isString() const
{
  return m_classType == H5T_STRING;
} // end of const bool HDF5DataTypeBase::isString() const

bool
HDF5DataTypeBase::isFloat() const
{
  return m_classType == H5T_FLOAT;
} // end of const bool HDF5DataTypeBase::isFloat() const

bool
HDF5DataTypeBase::isInteger() const
{
  return m_classType == H5T_INTEGER;
} // end of const bool HDF5DataTypeBase::isInteger() const

bool
HDF5DataTypeBase::isVariableString() const
{
  const htri_t isVariable = H5Tis_variable_str(m_id);

  if (isVariable <= 0) {
    return false;
  }
  return true;
} // end of const bool HDF5DataTypeBase::isVariableString() const

// ----------------------------------------------------------------------------
// Get the physical size of the file-based datatype.
// ----------------------------------------------------------------------------
size_t
HDF5DataTypeBase::getSize() const
{
  // -----------------------------------------------------------------------
  // Total size in bytes of a given data type in the file
  // Returns:
  //   > 0: success and is total size in bytes on disk
  //   = 0: failure
  // -----------------------------------------------------------------------
  const size_t dataSize = H5Tget_size(m_id);

  if (dataSize < 1) {
    throw std::runtime_error("H5Tget_size failure in HDF5DataTypeBase::getSize");
  }
  return dataSize;
} // end of const size_t HDF5DataTypeBase::getSize() const

// ----------------------------------------------------------------------------
// Get the native size of the memory-based datatype.
// ----------------------------------------------------------------------------
size_t
HDF5DataTypeBase::getNativeSize() const
{
  // -----------------------------------------------------------------------
  // Total size in bytes of a given data type in memory
  // Returns:
  //   > 0: success and is total size in bytes in memory
  //   = 0: failure
  // -----------------------------------------------------------------------
  const size_t dataSize = H5Tget_size(m_nativeId);

  if (dataSize < 1) {
    throw std::runtime_error("H5Tget_size nativeSize failure in HDF5DataTypeBase::getSize");
  }
  return dataSize;
} // end of const size_t HDF5DataTypeBase::getNativeSize() const

// ----------------------------------------------------------------------------
// Protected Definitions
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Open Datatype -- Virutal method to override with call to get datatype of
// the Attribute or Dataset being used.
// ----------------------------------------------------------------------------
void
HDF5DataTypeBase::openDataType(const hid_t t_id)
{
  // Override with Attribute...
  // const hid_t dataTypeId = H5Aget_type(t_id);
  // -- or --
  // Dataset function call
  // const hid_t dataTypeId = H5Dget_type(t_id);
  // then following error check
  // if (dataTypeId <= 0)
  // {
  //  throw std::runtime_error("H5Aget_type failure in HDF5AttrType::open");
  // }
  // setId(dataTypeId);
} // void HDF5DataTypeBase::openDataType()

// ----------------------------------------------------------------------------
// Set to opened file-based datatype identifier
// ----------------------------------------------------------------------------
void
HDF5DataTypeBase::setId(const hid_t t_id)
{
  m_id = t_id;
} // end of void HDF5DataTypeBase::setId(const hid_t t_id)

// ----------------------------------------------------------------------------
// Set file-based datatype class type for instance to cache for look-up methods.
// ----------------------------------------------------------------------------
void
HDF5DataTypeBase::setClassType()
{
  // -----------------------------------------------------------------------
  // Data type class identifier such as H5T_DOUBLE
  //   > 0: success and contains datatype identifier
  //   = 0: success and contains datatype identifier
  //   < 0: failure which should contain H5T_NO_CLASS (-1)
  // -----------------------------------------------------------------------
  m_classType = H5Tget_class(m_id);
  if (m_classType < 0) {
    m_classType = H5T_NO_CLASS;
  }
} // end of void HDF5DataTypeBase::setClassType()

// ----------------------------------------------------------------------------
// Open Native (memory) based datatype.
// ----------------------------------------------------------------------------
void
HDF5DataTypeBase::openNativeType()
{
  // -----------------------------------------------------------------------
  // H5Tget_native_type: Copy of native (memory-based) datatype.
  // H5Tclose to prevent leaks. Returns:
  //   > 0: success and is datatype identifier in file
  //   = 0: success (confirmed by HDF)
  //   < 0: failure
  // -----------------------------------------------------------------------
  m_nativeId = H5Tget_native_type(m_id, H5T_DIR_DEFAULT);
  if (m_nativeId < 0) {
    throw std::runtime_error("H5Tget_native_type failure in HDF5DataTypeBase::openNativeType");
  }
} // end of void HDF5DataTypeBase::openNativeType()

// ****************************************************************************
//
// A T T R I B U T E    D A T A T Y P E    C L A S S    D E F I N I T I O N
//
// ****************************************************************************
// ----------------------------------------------------------------------------
// Constructor for HDF Attribute-based Data Types
// ----------------------------------------------------------------------------
HDF5AttrType::HDF5AttrType(const hid_t t_id)
{
  try
  {
    open(t_id);
  }
  catch (...)
  {
    close();
    throw;
  }
} // end of HDF5AttrType::HDF5AttrType(const hid_t t_id)

// ----------------------------------------------------------------------------
// Overriding open to get Attribute-based datatype.
// ----------------------------------------------------------------------------
void
HDF5AttrType::openDataType(const hid_t t_id)
{
  // -----------------------------------------------------------------------
  // H5Aget_type: Copy of attribute datatype in file. H5Tclose to prevent
  // leaks. Returns:
  //   > 0: success and is datatype identifier in file
  //   = 0: success but HDF throws as failure in C++ library (defaulting to C++)
  //   < 0: failure
  // -----------------------------------------------------------------------
  const hid_t dataTypeId = H5Aget_type(t_id);

  if (dataTypeId <= 0) {
    throw std::runtime_error("H5Aget_type failure in HDF5AttrType::open");
  }
  setId(dataTypeId);
} // end of void HDF5AttrType::open(const t_id)
} // namespace hdflib
