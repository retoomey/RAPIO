#include "rIOHDF5.h"

#include "rFactory.h"
#include "rIOURL.h"

#include <hdf5.h>

// Specializers.  Only one right now for ODIM data
#include "rODIMDataHandler.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOHDF5();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOHDF5::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the hdf5 C library to read DataGrids or MRMS RadialSets, etc.";
  return help;
}

void
IOHDF5::initialize()
{
  // Add specializers for 'what' in the hdf5 file
  // Add the default classes we handle...
  ODIMDataHandler::introduceSelf(this);
}

IOHDF5::~IOHDF5()
{ }

// FIXME: implement into the rdump/text output
namespace {
// Function to print attributes of a given object with safety checks
void
print_attributes(hid_t obj_id, const char * obj_name)
{
  int num_attrs = H5Aget_num_attrs(obj_id);

  if (num_attrs > 0) {
    std::cout << " Attributes of " << obj_name << " with " << num_attrs << "\n";
  }

  for (int i = 0; i < num_attrs; i++) {
    hid_t attr_id = H5Aopen_by_idx(obj_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
      fprintf(stderr, "    [Warning] Failed to open attribute %d of %s\n", i, obj_name);
      continue;
    }

    // Get the attribute name
    char attr_name[256];
    ssize_t name_len = H5Aget_name(attr_id, sizeof(attr_name), attr_name);
    if (name_len < 0) {
      fprintf(stderr, "    [Warning] Failed to get attribute name.\n");
      H5Aclose(attr_id);
      continue;
    }
    std::cout << "     - " << attr_name << ": ";

    hid_t attr_type        = H5Aget_type(attr_id);
    hid_t attr_space       = H5Aget_space(attr_id);
    H5T_class_t type_class = H5Tget_class(attr_type);

    // Get total number of elements
    hssize_t num_elements = H5Sget_simple_extent_npoints(attr_space);

    // Get the size of a single element
    size_t type_size = H5Tget_size(attr_type);

    //	std::cout << " size: " << num_elements << " * " << type_size <<
    //	" Bytes " << num_elements*type_size << " Class: " << type_class << "\n";


    // For a single value, print it out

    if (type_class == H5T_INTEGER) {
      if (num_elements == 1) {
        int value;
        if (H5Aread(attr_id, H5T_NATIVE_INT, &value) >= 0) {
          std::cout << value;
        }
      } else {
        std::cout << "INTEGER Array[" << num_elements << "]";
      }
    } else if (type_class == H5T_FLOAT) {
      if (num_elements == 1) {
        double value;
        if (H5Aread(attr_id, H5T_NATIVE_DOUBLE, &value) >= 0) {
          std::cout << value;
        }
      } else {
        std::cout << "FLOAT Array[" << num_elements << "]";
      }
    } else if (type_class == H5T_STRING) {
      if (num_elements == 1) {
        if (H5Tis_variable_str(attr_type)) {
          // FIXME: Untested Haven't found an example.
          // Handle variable-length string
          char * value = (char *) malloc(type_size + 1); // +1 for null terminator
          if (H5Aread(attr_id, attr_type, &value) >= 0) {
            value[type_size] = '\0';
            std::cout << "\"" << value << "\"";
            free(value);
          }
        } else {
          // char value[type_size+1]; // gcc actually allows this, but maybe not older version.
          char * value = (char *) malloc(type_size + 1); // +1 for null terminator
          if (H5Aread(attr_id, attr_type, value) >= 0) {
            value[type_size] = '\0';
            std::cout << "\"" << value << "\"";
            free(value); // Free variable-length string
          }
        }
      } else {
        std::cout << "STRING Array[" << num_elements << "]";
      }
    } else {
      if (num_elements == 1) {
        std::cout << "Unknown\n";
      } else {
        std::cout << "Unknown Array[" << num_elements << "]";
      }
    }

    std::cout << "\n";

    // Cleanup
    H5Tclose(attr_type);
    H5Sclose(attr_space);
    H5Aclose(attr_id);
  }
} // print_attributes

// Enhanced callback function to list objects and their attributes
herr_t
list_objects(hid_t loc_id, const char * name, const H5O_info2_t * info, void * op_data)
{
  if (info->type == H5O_TYPE_GROUP) {
    printf("Group: %s\n", name);
  } else if (info->type == H5O_TYPE_DATASET) {
    printf("Dataset: %s\n", name);
  } else if (info->type == H5O_TYPE_NAMED_DATATYPE) {
    printf("Named Datatype: %s\n", name);
  }

  // Open the object to access attributes
  hid_t obj_id = H5Oopen(loc_id, name, H5P_DEFAULT);

  if (obj_id >= 0) {
    print_attributes(obj_id, name);
    H5Oclose(obj_id);
  }

  return 0;
}
}

bool
IOHDF5::isHdf5File(const std::string& t_filename)
{
  // -----------------------------------------------------------------------
  // HDF5 C Interface to verify file is in HDF5 format.
  // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-IsHDF5
  //
  // Returns:
  // > 0: success and is HDF5 file
  // = 0: success and is not an HDF5 file (confirmed by HDF)
  // < 0: failure or file does not exist
  // -----------------------------------------------------------------------
  const htri_t status = H5Fis_hdf5(t_filename.c_str());

  if (status == 0) {
    LogSevere("Not a HDF5 file: " << t_filename << '\n');
    return false;
  } else if (status < 0) {
    LogSevere("Failed to determine if file is HDF5: " << t_filename << '\n');
    return false;
  }

  return true;
}

std::shared_ptr<DataType>
IOHDF5::createDataType(const std::string& params)
{
  URL url(params);

  LogInfo("HDF5 reader: " << url << "\n");
  std::shared_ptr<DataType> datatype = nullptr;

  const std::string filename = url.toString();
  hid_t hdfid = -1;

  try {
    if (isHdf5File(filename)) {
      // Try to open the file for reading.
      // FIXME: Could make generate HDF5 exception class
      hdfid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      if (hdfid < 0) {
        LogSevere("Failed to HDF5 open file: " << filename << "\n")
      } else {
        // FIXME: We could do a general HDF5 file DataType.  Might be nice at somo point.
        // For the moment, focusing on Opera Data Information Model (ODIM)
        std::string type = "ODIM";
        std::shared_ptr<IOSpecializer> fmt = IOHDF5::getIOSpecializer(type);
        if (fmt != nullptr) {
          std::map<std::string, std::string> keys;
          keys["HDF5_ID"]  = std::to_string(hdfid);
          keys["HDF5_URL"] = url.toString();
          datatype         = fmt->read(keys, nullptr);
          if (datatype) {
            datatype->postRead(keys);
          }
        }

        // Dump ability/test (debug for now)
        // H5Ovisit3(hdfid, H5_INDEX_NAME, H5_ITER_NATIVE, list_objects, NULL, H5O_INFO_ALL);
      }
    }
  }catch (const std::exception& e) {
    LogSevere("Error creating hdf5 datatype " << e.what() << "\n");
  }

  // Final close
  if (hdfid >= 0) {
    H5Fclose(hdfid);
  }

  return (datatype);
} // IOHDF5::createDataType

bool
IOHDF5::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  // FIXME: Could generalize HDF5 output files?
  LogSevere("Write ability not implemented currently for direct HDF5\n")
  return false;
} // IOHDF5::encodeDataType

// ---------------------------------------------------------------------------
// Determine if HDF5 object exists
// ---------------------------------------------------------------------------
bool
isObjectExists(const hid_t t_id,
  const std::string        & t_name,
  const hid_t              t_aplId)
{
  // -----------------------------------------------------------------------
  // HDF5 Interface to determine if object exists. Returns:
  // > 0: success and object exists
  // = 0: success and object does not exist (confirmed by HDF)
  // < 0: failure or object does not exist
  // -----------------------------------------------------------------------
  return H5Lexists(t_id, t_name.c_str(), t_aplId) > 0;
} // end of bool objectExists(const hid_t t_id,...
