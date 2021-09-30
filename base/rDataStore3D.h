#pragma once

#include "rDataStore.h"

namespace rapio
{
/** DataStore3D
 *
 *  Row-major order overlay to a DataStore.
 *
 * @author Robert Toomey
 */
template <typename T> class DataStore3D : public DataStore<T>
{
public:
  DataStore3D<T>() : DataStore<T>(){
    myX = myY = myZ = 0;
  }

  DataStore3D<T>(size_t x, size_t y, size_t z) : DataStore<T>(x * y * z)
  {
    myX = x;
    myY = y;
    myZ = z;
  }


  /** We want extra information on a resize, make it difficult
   * for people to resize without changing X and Y and Z*/
  void
  resize(size_t aSize, T fill) = delete;

  /** Replace it with one that forces x y z to be set */
  void
  resize(size_t x, size_t y, size_t z, T fill)
  {
    // Sneaky call deleted resize
    static_cast<DataStore<T> *>(this)->resize(x * y * z, fill);
    myX = x; // ROWS
    myY = y; // COLS
    myZ = z; // HEIGHT
  }

  /** We want extra information on a resize, make it difficult
   * for people to resize without changing X and Y*/
  void
  resize(size_t aSize) = delete;

  /** Replace it with one that forces x y to be set */
  void
  resize(size_t x, size_t y, size_t z)
  {
    // Sneaky call deleted resize
    static_cast<DataStore<T> *>(this)->resize(x * y * z);
    myX = x;
    myY = y;
    myZ = z;
  }

  /** Get number cells in X direction */
  inline size_t
  getX() const { return myX; }

  /** Get number cells in Y direction */
  inline size_t
  getY() const { return myY; }

  /** Get number cells in Z direction */
  inline size_t
  getZ() const { return myZ; }

  // Debated [][] access, but this requires a recursively defined
  // proxy object for multiple operator [] override and the overhead
  // of it.  And we can just have a stupid simple set function instead.

  /** Set data directly two dimension (Row-major order) */
  inline void
  set(size_t i, size_t j, size_t k, T v)
  {
    this->d[(i * myY) + j] = v;
  }

  /** Get data directly two dimension (Row-major order)
   * @param i row number
   * @param j col number
   * @param k height number
   */
  inline T
  get(size_t i, size_t j, size_t k) const { return this->d[(i * myY) + j]; }

private:

  /** Number of cells in X direction */
  size_t myX;

  /** Number of cells in Y direction */
  size_t myY;

  /** Number of cells in Z direction */
  size_t myZ;
};
}
