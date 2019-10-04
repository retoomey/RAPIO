#pragma once

#include <rUtility.h>

#include <map>
#include <string>
#include <rError.h>
#include <memory>

namespace rapio {
/**
 * This class "introduces" new subclasses that have been created to
 * the parent class.
 *
 * This is necessary for clients of your class to obtain your class by
 * invoking getReader, getFormatter, etc.
 *
 * A subclass will have to be introduced to the rest of the system
 * before it can interoperate with existing applications because all
 * the existing applications use only getReader, getFormatter, etc.
 * and can usually be extended by adding a std::string to their
 * configuration.
 *
 * <b> Note that all the std::strings that get passed into Introducer
 * are case-insensitive.  "VIL" is the same as "vil" or "viL". </b>
 *
 */
template <class X> class Factory : public Utility {
private:

  typedef typename std::map<std::string, std::shared_ptr<X> > MapType;
  static MapType myLookup;

public:

  /** Introduce new X to the rest of the system. */
  static void
  introduce(const std::string& name, std::shared_ptr<X> instance)
  {
    if (instance != 0) {
      myLookup[name] = instance;
    } else {
      LogSevere("invalid instance pointer for '" << name << "'\n");
    }
  }

  /** Get all the children for iteration */
  static MapType
  getAll()
  {
    return myLookup;
  }

  /** Is this type known to the factory? */
  static bool
  exists(const std::string& name)
  {
    return (myLookup.count(name) != 0);
  }

  /**
   * This is meant to be used only by the class X,
   * which normally will be a base-class type.
   */
  static std::shared_ptr<X>
  get(const std::string& name, const std::string& info = "")
  {
    std::shared_ptr<X> ret;
    typename MapType::const_iterator cur = myLookup.find(name);

    if (cur != myLookup.end()) {
      ret = cur->second;
    } else {
      LogSevere("No instance available for " << info << " and '" << name
                                             << "' size: " << myLookup.size() << "\n");
    }

    return (ret);
  }
};

/** Define the actual static member for the class */
template <class X>
typename Factory<X>::MapType Factory<X>::myLookup;
}
