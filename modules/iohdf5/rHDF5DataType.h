#pragma once

/**
 * @brief Access HDF5 datatype objects
 *
 * Access HDF5 Datatypes from the HDF5 Group using its C-library interface.
 * @see https://www.hdfgroup.org/solutions/hdf5/
 * @see https://portal.hdfgroup.org/display/HDF5/Datatypes
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5T.html
 *
 * @author Michael C. Taylor (OU-CIMMS/NSSL)
 * @date   2020-01-06
 *****************************************************************************/

// Local Libraries

// RAPIO Libraries

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries

namespace hdflib
{
// ****************************************************************************
//
// B A S E    D A T A T Y P E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Base Class to access the HDF5 Datatype interface.
 *
 * Base class uses HDF5 Datatype's C-library interface in connecting to HDF5
 * datatypes.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5T.html
 *
 * @note HDF5 stores data to a file-based on a datatype supplied by the writer.
 * The file-based datatype is specific to the writing machine's hardware.
 * Reading data from a file into memory requires HDF5 to convert the data
 * on the fly using a specified native datatype that is either supplied
 * by the caller or can be determined based on the base datatype class
 * of the file-based datatype. This native datatype must fall within
 * the same datatype class as the file-based datatype in order for HDF5
 * to successfully convert into memory. Example:
 * a. HDF5 data written to file using datatype of H5T_STD_U8LE.
 * b. Would like to read data into memory using native type of H5T_NATIVE_INT.
 * c. This is valid since H5T_STD_U8LE is of the same datatype class as
 *    H5T_NATIVE_INT. They both are of the H5T_INTEGER datatype class.
 *
 * @warning Class will throw std::runtime_error exceptions when failures
 * occur during process and interfacing with the HDF5 C-library interface.
 *****************************************************************************/
class HDF5DataTypeBase
{
public:

  /**
   * @brief Default constructor used to initialize the Base Class.
   *
   * Derived class will need to override as necessary and call 'open' method
   * in order to setup the instance properly. Derived class also needs
   * to define an 'openDataType' method to properly get the file-based
   * HDF5 datatype of the given Attribute or Dataset.
   *
   * If errors - @throw std::runtime_error.
   *************************************************************************/
  HDF5DataTypeBase();

  /**
   * @brief Default destructor for HDF5DataTypeBase.
   *
   * Closes (if open) datatype and native datatype and releases outstanding
   * HDF5 resources.
   *
   * No Exceptions thrown using this Destructor.
   *************************************************************************/
  virtual
  ~HDF5DataTypeBase();

  /**
   * @brief Open the file-based datatype and class type along with native datatype.
   *
   * This method will call the following:
   * - openDataType   - virtual method that is overridden to account for the
   *                    Attribute or Dataset specific HDF5 C-library function
   *                    call necessary to retrieve the object's datatype.
   * - openNativeType - opens the memory (native) datatype of the object.
   * - setClassType   - gets the file-based datatype class type and stores
   *                    in instance for quick look-up requirements.
   *
   * @throw std::runtime_error when either the datatype or native datatype fails.
   *
   * @param[in] t_id  HDF5 identifier of the Attribute or Group object.
   * @return Void
   *************************************************************************/
  void
  open(const hid_t t_id);

  /**
   * @brief Close opened datatype and native datatype and release resources.
   *
   * No Exceptions thrown using this method.
   *
   * @return boolean true if close successful; otherwise false.
   *************************************************************************/
  bool
  close();

  /**
   * @brief Get HDF5 identifier of file-based datatype.
   *
   * No Exceptions thrown using this method.
   *
   * @return identifier of file-based datatype.
   *************************************************************************/
  hid_t
  getId() const;

  /**
   * @brief Get HDF5 identifier of native (memory) based datatype.
   *
   * No Exceptions thrown using this method.
   *
   * @return identifier of native datatype.
   *************************************************************************/
  hid_t
  getNativeId() const;

  /**
   * @brief Get HDF5 class of file-based datatype.
   *
   * The HDF5 datatype class describes the layout of the data and used in
   * the conversion of data from file to memory and vice versa.
   *
   * No Exceptions thrown using this method.
   *
   * @return class type of file-based datatype.
   *************************************************************************/
  H5T_class_t
  getClassType() const;

  /**
   * @brief Determines if file-based datatype is of the H5T_STRING class.
   *
   * No Exceptions thrown using this method.
   *
   * @return true if string; otherwise false.
   *************************************************************************/
  bool
  isString() const;

  /**
   * @brief Determines if file-based datatype is of the H5T_INTEGER class.
   *
   * No Exceptions thrown using this method.
   *
   * @return true if integer; otherwise false.
   *************************************************************************/
  bool
  isInteger() const;

  /**
   * @brief Determines if file-based datatype is of the H5T_FLOAT class.
   *
   * No Exceptions thrown using this method.
   *
   * @return true if float; otherwise false.
   *************************************************************************/
  bool
  isFloat() const;

  /**
   * @brief Determines if file-based datatype is a variable-length string.
   *
   * Variable-length strings in HDF5 are handled differently than fixed-length.
   * So, important to know when working with one or the other.
   *
   * No Exceptions thrown using this method.
   *
   * @return true if variable-length string; otherwise false.
   *************************************************************************/
  bool
  isVariableString() const;

  /**
   * @brief Get the physical file-based size of the datatype.
   *
   * @throw std::runtime_error if failure to get size occurs.
   *
   * @return physical size of the file-based datatype.
   *************************************************************************/
  size_t
  getSize() const;

  /**
   * @brief Get the native memory-based size of the datatype.
   *
   * @throw std::runtime_error if failure to get size occurs.
   *
   * @return native size of the memory-based datatype.
   *************************************************************************/
  size_t
  getNativeSize() const;

protected:

  /**
   * @brief Virtual method to open file-based datatype based on Attribute or Dataset.
   *
   * Method should be overridden depending on whether Attribute or Dataset.
   * If attribute - should use HDF5 C-library H5Aget_type.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-GetType
   *
   * If dataset - should use HDF5 C-library H5Dget_type.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-GetType
   *
   * Throw a std::runtime_error exception if failure occurs in getting type.
   *
   * @param[in] t_id    HDF5 identifier attribute or dataset object.
   * @throw std::runtime_error if failure to get datatype
   *************************************************************************/
  virtual void
  openDataType(const hid_t t_id);

  /**
   * @brief Set the HDF5 identifier of the datatype.
   *
   * This method is called from the overriden virtual open method
   * in order to open the datatype based on whether Attribute or Dataset
   * and then set the identifier accordingly for use of the datatype object.
   *
   * No Exceptions thrown using this method.
   *
   * @param[in] t_id    HDF5 identifier attribute or dataset
   * @return Void
   *************************************************************************/
  void
  setId(const hid_t t_id);

  /**
   * @brief Set the class type of the file-based datatype opened.
   *
   * Class type is necessary in order to determine if the file-based
   * datatype can be successfully converted to the caller-based
   * native datatype.
   *
   * No Exceptions thrown using this method.
   *
   * @return Void
   *************************************************************************/

  void
  setClassType();

  /**
   * @brief Open native (memory-based) datatype.
   *
   * Memory-based (native) datatype is necessary in order for HDF5 to
   * understand how to convert a file-based datatype value into the HDF5
   * memory-space.
   *
   * @throw std::runtime_error on failure to open native datatype object.
   *
   * @return Void
   *************************************************************************/
  void
  openNativeType();

private:
  /** HDF5 identifier of the file-based datatype */
  hid_t m_id;

  /** HDF5 identifier of the native (memory) based datatype */
  hid_t m_nativeId;

  /** HDF5 class type of the file-based datatype */
  H5T_class_t m_classType;
};

// ****************************************************************************
//
// A T T R I B U T E    D A T A T Y P E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class to access the Attribute-specific HDF5 Datatype interface.
 *
 * Uses the HDF5 Attribute and Datatype's C-library interface in connecting
 * to HDF5 attribute datatypes.
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-GetType
 *
 *************************************************************************/
class HDF5AttrType : public HDF5DataTypeBase
{
public:

  /**
   * @brief Default constructor to access HDF5 Attribute datatype objects.
   *
   * @throw std::runtime_error if failure to construct occurs.
   *
   * @param[in] t_id      HDF5 identifier of the Attribute.
   *************************************************************************/
  explicit
  HDF5AttrType(const hid_t t_id);

protected:

  /**
   * @brief Open Attribute-based datatype for use by instance.
   *
   * Attribute-based datatype is obtained via HDF5 C-library: H5Aget_type.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-GetType
   *
   * @throw std::runtime_error if failure to get datatype
   *************************************************************************/
  void
  openDataType(const hid_t t_id);
};
} // end of namespace hdflib
