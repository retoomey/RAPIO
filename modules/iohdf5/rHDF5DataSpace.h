#pragma once

/**
 * @brief Access the HDF5 dataspaces describing dimensionality of data.
 *
 * Access HDF5 Dataspaces from the HDF5 Group using its C-library interface.
 * @see https://www.hdfgroup.org/
 * @see https://portal.hdfgroup.org/display/HDF5/Dataspaces
 * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html
 *
 * @warning Constructors and methods can throw following exceptions:
 * @throw std::runtime_error
 *
 * Constructors and methods that throw exceptions are clearly marked.
 *
 * @author Michael C. Taylor (OU-CIMMS/NSSL)
 * @date   2020-01-06
 *****************************************************************************/

// Local Libraries

// 3rd Party Libraries
#include <hdf5.h>

// System Libraries
#include <string>
#include <vector>

namespace hdflib
{
// ****************************************************************************
//
// B A S E    D A T A S P A C E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Base class to access HDF5 dataspace data interface
 *
 * Uses the HDF5 Group's C-library interface in connecting to HDF5 dataspaces.
 * @see https://portal.hdfgroup.org/display/HDF5/Dataspaces
 *
 * This class and its methods can throw the following Exception:
 * @throw std::runtime_error when failures occur in interfacing with the HDF5 C-library.
 *
 * Derived classes are:
 * @see HDF5AttributeSpace::HDF5AttributeSpace
 * @see HDF5DatasetSpace::HDF5DatasetSpace
 *
 */
class HDF5DataSpaceBase
{
public:

  /**
   * @brief Default constructor for base
   *
   * The derived Dataset or Attribute dataspace class overrides constructor
   * to use their appropriate C-library get_space function to obtain the
   * HDF5 dataspace.
   *
   * No Exception thrown using this Constructor.
   *
   */
  HDF5DataSpaceBase();

  // -----------------------------------------------------------------------
  // Destructors
  // -----------------------------------------------------------------------

  /**
   * @brief Default destructor
   *
   * Closes dataspace (if open) and releases outstanding HDF5 resources.
   *
   * No Exception thrown using this Destructor
   *************************************************************************/
  virtual
  ~HDF5DataSpaceBase();

  /**
   * @brief Close opened dataspace and release resources.
   *
   * Closes dataspace (if open) using HDF5 C-library H5Sclose.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-Close
   *
   * No Exception thrown using this method.
   *
   * @return true if close successful; otherwise false
   *************************************************************************/
  bool
  close();

  /**
   * @brief Get dataspace identifier of opened dataspace.
   *
   * No Exception thrown using this method.
   *
   * @return dataspace identifier
   *************************************************************************/
  hid_t
  getId() const;

  /**
   * @brief Determines whether opened dataspace is simple or complex
   *
   * Determines simple dataspace via HDF5 C-library H5Sis_simple.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-IsSimple
   *
   * @warning Only simple dataspaces are supported at this time
   *
   * No Exception thrown using this method.
   *
   * @return true if simple dataspace;otherwise false
   *************************************************************************/
  bool
  isSimple() const;

  /**
   * @brief Get the number of elements in the opened dataspace
   *
   * Determines element count via HDF5 C-library H5Sget_simple_extent_npoints.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-ExtentNpoints
   *
   * @throw std::runtime_error if failure in calling HDF5 C-library function
   *
   * @return number of elements
   *************************************************************************/
  size_t
  getElementCount() const;

  /**
   * @brief Get the number of dimensions (rank) in the opened dataspace
   *
   * Determines rank via HDF5 C-library H5Sget_simple_extent_ndims.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-ExtentNdims
   *
   * @throw std::runtime_error if failure in calling HDF5 C-library function
   *
   * @return number of dimensions (rank)
   *************************************************************************/
  size_t
  getRank() const;

  /**
   * @brief Get the size and max size of each dimension in dataspace
   *
   * Determines size and max of each dimension via HDF5 C-library H5Sget_simple_extent_dims.
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-ExtentDims
   *
   * @throw std::runtime_error if failure in calling HDF5 C-library function.
   *
   * @param[in] t_rank    rank (number of dimensions) in opened dataspace
   * @return vector containing size and max size of each dimension in dataspace
   *************************************************************************/
  const std::vector<hsize_t>
  getDimensions(const size_t t_rank) const;

protected:

  /**
   * @brief Set the dataspace id to an already opened dataspace.
   *
   * No Exceptions are thrown using this method.
   *
   * @param t_id[in]  HDF5 identifier of the dataspace.
   * @return Void
   */
  void
  setId(const hid_t t_id);

private:
  /** dataspace identifier */
  hid_t m_id;
}; // end of HDF5DataSpaceBase

// ****************************************************************************
//
// A T T R I B U T E    D A T A S P A C E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class used to access Attribute-based HDF5 dataspaces.
 */
class HDF5AttributeSpace : public HDF5DataSpaceBase
{
public:

  /**
   * @brief Derived constructor to access HDF5 attribute datspace.
   *
   * Constructor used to access HDF5 attribute dataspace interface. Derived from
   * HDF5DataSpaceBase class.
   * @see HDF5DataSpaceBase::HDF5DataSpaceBase
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-GetSpace
   *
   * @throw std::runtime_error if failure in calling H5Aget_space C-library function
   *
   * @param[in] t_id    HDF5 attribute id used in H5Aget_space call to open dataspace
   */
  explicit
  HDF5AttributeSpace(const hid_t t_id);
}; // end of class AttributeSpace : public HDF5DataSpaceBase

// ****************************************************************************
//
// D A T A S E T    D A T A S P A C E    C L A S S    D E C L A R A T I O N
//
// ****************************************************************************

/**
 * @brief Class used to access Dataset-based HDF5 dataspaces.
 */
class HDF5DatasetSpace : public HDF5DataSpaceBase
{
public:

  /**
   * @brief Derived constructor to access HDF5 dataset datspace.
   *
   * Constructor used to access HDF5 dataset dataspace interface. Derived from
   * HDF5DataSpaceBase class.
   * @see HDF5DataSpaceBase::HDF5DataSpaceBase
   * @see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-GetSpace
   *
   * @throw std::runtime_error if failure in calling H5Dget_space C-library function
   *
   * @param[in] t_id    HDF5 dataset id used in H5Aget_space call to open dataspace
   */
  explicit
  HDF5DatasetSpace(const hid_t t_id);
}; // end of class DatasetSpace : public HDF5DataSpaceBase
} // end of namespace hdflib
