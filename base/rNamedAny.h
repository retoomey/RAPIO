#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rError.h>
#include <rBOOST.h>

#include <map>
#include <string>
#include <memory>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

/** Store an any object possibly as smart ptr, etc.
 *
 * @author Robert Toomey
 */
class Any : public Data
{
public:
  /** Create an Any */
  Any()
  { }

  // -----------------------------------------
  // Store as a wrapped smart ptr

  /** Get data we store */
  #if 0
  template <typename T>
  std::shared_ptr<T>
  getSP() const
  {
    // the cheese of wrapping vector around shared_ptr
    try{
      auto back = boost::any_cast<std::shared_ptr<T> >(myData);
      return back;
    }catch (const boost::bad_any_cast&) {
      return nullptr;
    }
  }

  /** Is this data this type? */
  template <typename T>
  bool
  isSP() const
  {
    return (myData.type() == typeid(std::shared_ptr<T> ));
  }

  #endif // if 0

  // -----------------------------------------
  // Store as an something, but return wrapped optional

  /** Get data we store as a boost optional */
  template <typename T>
  boost::optional<T>
  get() const
  {
    try{
      auto back = boost::any_cast<T>(myData);
      return boost::optional<T>(back);
    }catch (const boost::bad_any_cast&) { }
    return boost::none;
  }

  /** Is this data this type? */
  template <typename T>
  bool
  is() const
  {
    return (myData.type() == typeid(T));
  }

  /** Set the data we store */
  void
  set(boost::any&& in)
  {
    myData = std::move(in);
  }

  /** Clone the data (for raw types) */
  boost::any
  getData() const { return myData; }

protected:

  /** We can store ANYTHING!!!!! */
  boost::any myData;
};

/** Store a name to std::shared_ptr OR direct of anything pair.
 *
 * @author Robert Toomey
 */
class NamedAny : public Any
{
public:
  /** Create a named object */
  NamedAny(const std::string& name) : myName(name)
  { }

  /** Get the name of this named any */
  const std::string&
  getName() const { return myName; }

  /** Set the name of this named any */
  void
  setName(const std::string& s){ myName = s; }

protected:

  /** Name of this data */
  std::string myName;
};

/** A mapping class for NamedAny objects.
 *
 * @author Robert Toomey
 */
class NamedAnyList : public Data
{
public:

  /** Deep clone the NamedAnyList.  Note this works for basic types
   * like string, float, etc. and the Array shared_ptr storage.
   */
  std::shared_ptr<NamedAnyList>
  Clone();

  /** Allow begin to iterate through our elements */
  NamedAny *
  begin()
  {
    return &myAttributes[0];
  }

  /** Allow end to iterate through our elements */
  NamedAny *
  end()
  {
    return &myAttributes[myAttributes.size()]; // note it's one past end
  }

  /** Return index into the storage for this name */
  int
  index(const std::string& name) const;

  /** Add something with given key */
  template <typename T>
  void
  put(const std::string& name, const T& value)
  {
    int i = index(name);

    // Either set the old one to new data....
    if (i != -1) {
      myAttributes[i].setName(name);
      myAttributes[i].set(value);
    } else {
      // Or add a new node ...
      NamedAny g(name);
      g.set(value);

      myAttributes.push_back(g);
    }
  }

  /** Remove something with given key */
  void
  remove(const std::string& name)
  {
    int i = index(name);

    if (i != -1) {
      std::swap(myAttributes[i], myAttributes.back());
      myAttributes.pop_back();
    }
  }

  /** Get data we store as an optional pointer.
   * If not boost::none, can use * to get to the
   * field.
   * Note: This is a pointer and not the actual
   * typename.
   */
  template <typename T>
  boost::optional<T>
  get(const std::string& name) const
  {
    int i = index(name);

    if (i != -1) {
      return myAttributes[i].get<T>();
    }
    return boost::none;
  }

  /** Get the size of the attributes */
  size_t
  size() const
  {
    return myAttributes.size();
  }

  // ----------------------------------------------
  // Convenience routines for common types
  // Debated is vs have with DataArray and
  // AttributeDataType classes.

  /** Get a string */
  inline bool
  getString(const std::string& name, std::string& out) const
  {
    auto s = get<std::string>(name);

    if (s) {  out = *s; return true; }
    return false;
  }

  /** Set a string */
  inline void
  setString(const std::string& name, const std::string& in)
  {
    put<std::string>(name, in);
  }

  /** Get a double, flexible on casting */
  inline bool
  getDouble(const std::string& name, double& out) const
  {
    const auto at = index(name);

    if (at != -1) {
      const NamedAny& value = myAttributes[at];
      auto d = value.get<double>();
      if (d) { out = *d; return true; }

      // Multipleconversion attempts
      // since boost::any stores actual type.  We should have
      // all the types created by netcdf at least here
      auto f = value.get<float>();
      if (f) { out = *f; return true; }
      auto i = value.get<int>();
      if (i) { out = *i; return true; }
      auto l = value.get<long>();
      if (l) { out = *l; return true; }
    }
    return false;
  }

  /** Set a double */
  inline void
  setDouble(const std::string& name, double in)
  {
    put<double>(name, in);
  }

  /** Get a float, flexible on casting */
  inline bool
  getFloat(const std::string& name, float& out) const
  {
    const auto at = index(name);

    if (at != -1) {
      const NamedAny& value = myAttributes[at];
      auto f = value.get<float>();
      if (f) { out = *f; return true; }

      // Multipleconversion attempts
      // since boost::any stores actual type.  We should have
      // all the types created by netcdf at least here
      auto d = value.get<double>();
      if (d) { out = *d; return true; }
      auto i = value.get<int>();
      if (i) { out = *i; return true; }
      auto l = value.get<long>();
      if (l) { out = *l; return true; }
    }
    return false;
  }

  /** Set a float */
  inline void
  setFloat(const std::string& name, float in)
  {
    put<float>(name, in);
  }

  /** Get a long, flexible on casting */
  inline bool
  getLong(const std::string& name, long& out) const
  {
    const auto at = index(name);

    if (at != -1) {
      const NamedAny& value = myAttributes[at];
      auto l = value.get<long>();
      if (l) { out = *l; return true; }

      // Multipleconversion attempts
      // since boost::any stores actual type.  We should have
      // all the types created by netcdf at least here
      auto i = value.get<int>();
      if (i) { out = *i; return true; }
    }
    return false;
  }

  /** Set a long in global attributes */
  inline void
  setLong(const std::string& name, long in)
  {
    put<long>(name, in);
  }

protected:

  /** Stored attributes */
  std::vector<NamedAny> myAttributes;
};

typedef NamedAnyList DataAttributeList;
}
