#pragma once

#include "rDataStore.h"

namespace rapio
{
/** DataStore2D
 *
 *  Row-major order overlay to a DataStore.
 *
 * @author Robert Toomey
 */
template <typename T> class DataStore2D : public DataStore<T>
{
public:
  DataStore2D<T>() : DataStore<T>(){
    myX = myY = 0;
  }

  DataStore2D<T>(size_t x, size_t y) : DataStore<T>(x * y)
  {
    myX = x;
    myY = y;
  }


  /** We want extra information on a resize, make it difficult
   * for people to resize without changing X and Y*/
  void
  resize(size_t aSize, T fill) = delete;

  /** Replace it with one that forces x y to be set */
  void
  resize(size_t x, size_t y, T fill)
  {
    // Sneaky call deleted resize
    static_cast<DataStore<T> *>(this)->resize(x * y, fill);
    myX = x; // ROWS
    myY = y; // COLS
  }

  /** We want extra information on a resize, make it difficult
   * for people to resize without changing X and Y*/
  void
  resize(size_t aSize) = delete;

  /** Replace it with one that forces x y to be set */
  void
  resize(size_t x, size_t y)
  {
    // Sneaky call deleted resize
    static_cast<DataStore<T> *>(this)->resize(x * y);
    myX = x;
    myY = y;
  }

  /** Get number cells in X direction */
  inline size_t
  getX() const { return myX; }

  /** Get number cells in Y direction */
  inline size_t
  getY() const { return myY; }

  /** Proxy class for implementing [][] */
  template <typename S>
  class DataStoreDim : public Data {
public:
    DataStoreDim(DataStore2D<S>& x, int iin, size_t yin) : parent(x), i(iin), y(yin){ }

    /** [][] j operator */
    S&
    operator [] (int j)
    {
      return parent.d[(i * y) + j];
    }

private:
    DataStore2D<S>& parent;
    int i;
    size_t y;
  };

  /** First [] of the 2D operator[][] */
  DataStoreDim<T>
  operator [] (int i)
  {
    return DataStoreDim<T>(*this, i, myY);
  }

  /** Get data directly two dimension (Row-major order)
   * @param i row number
   * @param j col number
   */
  inline T
  get(size_t i, size_t j) const { return this->d[(i * myY) + j]; }

  /** Get dimensions */
  std::vector<size_t>
  shape()
  {
    return { myX, myY };
  }

private:

  /** Number of cells in X direction */
  size_t myX;

  /** Number of cells in Y direction */
  size_t myY;
};
}
