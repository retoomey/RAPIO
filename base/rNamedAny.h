#pragma once

#include <rConstants.h>
#include <rError.h>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/any.hpp>
#include <boost/optional.hpp>
BOOST_WRAP_POP

#include <map>
#include <string>
#include <string_view>
#include <memory>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

/** Store an any object possibly as smart ptr, etc.
 *
 * @author Robert Toomey
 * @ingroup rapio_data
 * @brief Stores anything abstractly.
 */
class Any
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
 * @ingroup rapio_data
 * @brief An Any subclass that has a name
 */
class NamedAny : public Any
{
public:
  /** Create a named object */
  NamedAny(std::string_view name) : myName(name)
  { }

  /** Get the name of this named any */
  const std::string&
  getName() const { return myName; }

  /** Set the name of this named any */
  void
  setName(std::string_view s){ myName = s; }

protected:

  /** Name of this data */
  std::string myName;
};

/** A mapping class for NamedAny objects.
 *
 * @author Robert Toomey
 * @ingroup rapio_data
 * @brief Stores a list of NamedAny for lookup.
 */
class NamedAnyList
{
public:

  /** Deep clone the NamedAnyList.  Note this works for basic types
   * like string, float, etc. and the Array shared_ptr storage.
   */
  std::shared_ptr<NamedAnyList>
  Clone() const;

  /** Allow begin to iterate through our elements (mutable) */
  NamedAny *
  begin()
  {
    return myAttributes.data();
  }

  /** Allow end to iterate through our elements (mutable) */
  NamedAny *
  end()
  {
    return myAttributes.data() + myAttributes.size();
  }

  /** Allow begin to iterate through our elements  (const) */
  const NamedAny *
  begin() const
  {
    return myAttributes.data();
  }

  /** Allow end to iterate through our elements (const) */
  const NamedAny *
  end() const
  {
    return myAttributes.data() + myAttributes.size();
  }

  /** Return index into the storage for this name */
  int
  index(std::string_view name) const;

  /** Add something with given key */
  template <typename T>
  void
  put(std::string_view name, T value)
  {
    int i = index(name);

    // Either set the old one to new data....
    if (i != -1) {
      myAttributes[i].set(std::move(value));
    } else {
      // Or add a new node ...
      NamedAny g(name);
      g.set(std::move(value));

      myAttributes.push_back(std::move(g)); // Move to avoid copy
    }
  }

  /** Remove something with given key */
  void
  remove(std::string_view name)
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
  get(std::string_view name) const
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

  /** Are we empty? */
  bool
  empty() const
  {
    return myAttributes.empty();
  }

  // ----------------------------------------------
  // Convenience routines for common types
  // Debated is vs have with DataArray and
  // AttributeDataType classes.

  /** Get a string */
  inline bool
  getString(std::string_view name, std::string& out) const
  {
    auto s = get<std::string>(name);

    if (s) {  out = *s; return true; }
    return false;
  }

  /** Set a string */
  inline void
  setString(std::string_view name, std::string_view in)
  {
    put<std::string>(name, std::string(in));
  }

  /** Get a double, flexible on casting */
  inline bool
  getDouble(std::string_view name, double& out) const
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
  setDouble(std::string_view name, double in)
  {
    put<double>(name, in);
  }

  /** Get a float, flexible on casting */
  inline bool
  getFloat(std::string_view name, float& out) const
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
  setFloat(std::string_view name, float in)
  {
    put<float>(name, in);
  }

  /** Get a long, flexible on casting */
  inline bool
  getLong(std::string_view name, long& out) const
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
  setLong(std::string_view name, long in)
  {
    put<long>(name, in);
  }

protected:

  /** Stored attributes */
  std::vector<NamedAny> myAttributes;
};

typedef NamedAnyList DataAttributeList;
}
