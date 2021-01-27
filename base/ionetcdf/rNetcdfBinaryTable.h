#pragma once

#include <rIONetcdf.h>

class NcVar;

namespace rapio {
class BinaryTable;

/** Handles the construction of BinaryTable object from the netcdf file.
 *  @see NetcdfBuilder
 */
class NetcdfBinaryTable : public NetcdfType {
public:

  /** Write DataType from given ncid */
  virtual bool
  write(int                    ncid,
    std::shared_ptr<DataType>  dt,
    std::shared_ptr<PTreeNode> dfs)
  override;

  /** The way to obtain the object.
   *  @params ncfile An open NetcdfFile object.
   *  prms   Only the file name (first param) is needed.
   */
  virtual std::shared_ptr<DataType>
  read(const int ncid,
    const URL    & loc)
  override;

  virtual
  ~NetcdfBinaryTable();

  static void
  introduceSelf();

  /** Write out a BinaryTable. */
  static bool
  write(int     ncid,
    BinaryTable & binaryTable,
    const float missing,
    const float rangeFolded);

private:

  std::string filename;

  // void fillData( LatLonHeightGrid& grid,
  //               NcVar* data_var, int dim1, int dim2 , int dim3 );
};
}
