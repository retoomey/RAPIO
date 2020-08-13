#pragma once

#include <rUtility.h>

#include <map>
#include <string>
#include <rError.h>
#include <rOS.h>

namespace rapio {
/**
 * Factory that has a static method to create a unique
 * instance of some object. It is simplier than Factory
 * because it doesn't store an active object, which works
 * to save memory for the simplier lookup classes.
 * FIXME: It's possible maybe to combine with Factory
 *
 * @author Robert Toomey
 *
 */
template <class X> class StaticMethodFactory : public Utility {
private:

  /** Define what the create function will return, a shared ptr to
   * the created object */
  typedef std::shared_ptr<X>(*FuncPointer)();

  /** The map is a name to static method lookup */
  typedef typename std::map<std::string, FuncPointer> MapType;

  /** Only one global factory per type is allowed */
  static MapType myLookup;

public:

  /** Introduce new X to the rest of the system. */
  static void
  introduce(const std::string& name, FuncPointer instance)
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
  static FuncPointer
  get(const std::string& name, const std::string& info = "")
  {
    FuncPointer ret;

    typename MapType::const_iterator cur = myLookup.find(name);

    if (cur != myLookup.end()) { // Normally fairly quick...
      ret = cur->second;
    } else {
      ret = nullptr;
    }

    // Factory failed to find or load object
    if (ret == nullptr) {
      LogSevere("No instance available for " << info << " and '" << name
                                             << "' size: " << myLookup.size() << "\n");
    }

    return (ret);
  } // get
};

/** Define the actual static member for the class */
template <class X>
typename StaticMethodFactory<X>::MapType StaticMethodFactory<X>::myLookup;
}
