#pragma once

#include <rIO.h>

#include <vector>
#include <memory>

namespace rapio {
/**
 * The class Buffer provides a convenient way to pass around data
 * buffers without worrying about memory leaks.
 *
 * Buffer is implemented as holding a smart pointer to a std::vector of
 * characters.
 *
 * FIXME: This is the same as a DataStore<char>
 */
class Buffer : public IO {
public:

  /** Returns a pointer to the internal array of bytes. */
  std::vector<char>&
  data()
  {
    return (*myData);
  }

  /** Returns a constant pointer to the internal array of bytes. */
  const std::vector<char>&
  data() const
  {
    return (*myData);
  }

  /** Returns current buffer length.  Returns zero if the buffer is empty. */
  int
  getLength() const
  {
    return (myData->size());
  }

  /** Identical to getLength(). */
  int
  size() const
  {
    return (myData->size());
  }

  bool
  empty() const
  {
    return (myData->empty());
  }

  void
  assign(const char * buf, int buflen)
  {
    myData->clear();
    myData->insert(myData->end(), buf, buf + buflen);
  }

  /** Construct out of passed in buffer. Makes a copy of the buffer, so
   * you can safely delete buf when you are done. */
  Buffer(const char * buf, int buflen)
    :      myData(new std::vector<char>(buf, buf + buflen))
  { }

  /** Construct out of passed in std::vector of char */
  explicit Buffer(const std::vector<char>& buf)
    :      myData(new std::vector<char>(buf))
  { }

  /** Construct a buffer from another buffer between two indices. */
  Buffer(const Buffer& orig, int st_ind, int end_ind)
    :      myData(new std::vector<char>(&(orig.data()[st_ind]),
      &(orig.data()[end_ind])))
  { }

  /** Construct a buffer of passed in length. Contents of buffer are
   * uninitialized. */
  explicit Buffer(int buflen) : myData(new std::vector<char>(buflen))
  { }

  /** Appends another buffer to this one. */
  void
  append(const Buffer& s)
  {
    myData->insert(myData->end(), s.myData->begin(), s.myData->end());
  }

  /** Creates empty buffer. */
  Buffer() : myData(new std::vector<char>(0))
  { }

  /** Makes a wholly separate copy. */
  Buffer
  clone() const
  {
    return (Buffer(this->data()));
  }

  /** operator [] allows you to use Buffer just like an array.
   * e.g:  Buffer buf(...);  buf[3] = 'c'; will set the 3rd char. */
  char&
  operator [] (std::size_t n)
  {
    return ((*myData)[n]);
  }

  /** operator [] allows you to use Buffer just like an array.
   * e.g:  Buffer buf(...);  char c = buf[3]; will get the 3rd char. */
  const char&
  operator [] (std::size_t n) const
  {
    return ((*myData)[n]);
  }

  /** Returns a pointer to the beginning of the data location. */
  const char *
  begin() const
  {
    return (&((*myData)[0]));
  }

  /** Returns a pointer to the beginning of the data location. */
  char *
  begin()
  {
    return (&((*myData)[0]));
  }

protected:

  std::shared_ptr<std::vector<char> > myData;
}
;
}
