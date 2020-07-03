#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rError.h>

#include <map>
#include <string>
#include <memory>

#include <boost/any.hpp>
#include <boost/optional.hpp>

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
  template <typename T>
  std::shared_ptr<T>
  getSP()
  {
    // the cheese of wrapping vector around shared_ptr
    try{
      auto back = boost::any_cast<std::shared_ptr<T> >(myData);
      return back;
    }catch (boost::bad_any_cast) {
      return nullptr;
    }
  }

  /** Is this data this type? */
  template <typename T>
  bool
  isSP()
  {
    return (myData.type() == typeid(std::shared_ptr<T> ));
  }

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
    }catch (boost::bad_any_cast) { }
    return boost::none;
  }

  /** Is this data this type? */
  template <typename T>
  bool
  is()
  {
    return (myData.type() == typeid(T));
  }

  /** Set the data we store */
  void
  set(boost::any&& in)
  {
    myData = std::move(in);
  }

protected:

  /** We can store ANYTHING!!!!! */
  boost::any myData;
};

/** Store a name to std::shared_ptr OR direct of anything pair.
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
  getName(){ return myName; }

  /** Set the name of this named any */
  void
  setName(const std::string& s){ myName = s; }

protected:

  /** Name of this data */
  std::string myName;
};

/** A mapping class for NamedAny objects*/
class NamedAnyList : public Data
{
public:

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
  index(const std::string& name) const
  {
    size_t count = 0;

    for (auto i:myAttributes) {
      if (i.getName() == name) {
        return count; // always +
      }
      count++;
    }
    return -1;
  }

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

  /** Get data we store as an optional */
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
  size()
  {
    return myAttributes.size();
  }

protected:

  /** Stored attributes */
  std::vector<NamedAny> myAttributes;
};
}
