#pragma once

#include <rDataType.h>

#include <vector>

namespace rapio {
/** A DataType that holds N other DataTypes.
 * Its only purpose is to hold other DataTypes that
 * handle their own data.
 *
 * FIXME: We'll need to add clone, delete, etc. to make
 * this a fully useable class.
 */
class MultiDataType : public DataType {
public:

  /** Create empty MultiDataType */
  MultiDataType(bool groupWrite = false) : myGroupWrite(groupWrite){ };

  /** Public API for users to create a MultiDataType quickly */
  static std::shared_ptr<MultiDataType>
  Create(bool groupWrite = false);

  /** Destroy a MultiDataType */
  virtual ~MultiDataType(){ }

  /** Given a shared_ptr of a MultiDataType, return simpliest available.
   * So for example a nullptr becomes a nullptr, a MultiDataType with a
   * single DataType becomes that DataType. More than one just returns the
   * MultiDataType.
   */
  static std::shared_ptr<DataType>
  Simplify(std::shared_ptr<MultiDataType>& m);

  /** Set the write mode.  The default mode is to break up the
   * MultiDataType into N DataTypes, sending each independently to
   * the write module.  However, some writers have the ability to
   * handle MultiDataTypes.  IOIMAGE can take N DataTypes and create
   * a single output image from the data from all of them.  In this
   * case we can set this flag to send the entire group to the
   * writer module. */
  void setSendToWriterAsGroup(bool flag){ myGroupWrite = flag; }

  /** Get the current write as group flag, determine how it's
   * send to writer modules */
  bool getSendToWriterAsGroup(){ return myGroupWrite; }

  /** Add a DataType to our collection */
  void
  addDataType(std::shared_ptr<DataType> add);

  /** Get DataType at a given index */
  std::shared_ptr<DataType>
  getDataType(size_t i);

  /** Get the number of DataTypes */
  size_t
  size();

protected:

  /** Hold onto a collection of DataTypes */
  std::vector<std::shared_ptr<DataType> > myDataTypes;

  /** Writer mode flag */
  bool myGroupWrite;
};
}
