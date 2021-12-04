#pragma once

#include <rUtility.h>

#include <map>
#include <string>
#include <rError.h>
#include <rOS.h>
#include <memory>

namespace rapio {
/**
 * Factory create a name to object lookup based on type.
 * It has the ability to lazy load from dynamic libraries
 * on demand to save runtime memory.
 *
 * FIXME: lazy unload after so many calls?
 *
 * @author Robert Toomey
 *
 */
template <class X> class Factory : public Utility {
private:
  /** Simple private lookup for late loaded dynamic factories */
  template <class Z> class FactoryLookup : public Utility {
public:
    /** Std containers */
    FactoryLookup(){ };

    /** Construct with default keys */
    FactoryLookup(const std::string& m, const std::string& mname)
      : module(m), methodname(mname), loaded(false), stored(nullptr)
    { }

    /** Dynamic library lib.so */
    std::string module;

    /** Creation method to call */
    std::string methodname;

    /** Have we attempt to load already? */
    bool loaded;

    /** Alias keys for lookup. */
    std::vector<std::string> alias;

    /** Also store the factory pointer to avoid multiple creation. */
    std::shared_ptr<X> stored;
  };

private:

  /** Lookup storing key to actual object */
  typedef typename std::map<std::string, std::shared_ptr<X> > MapType;
  static MapType myLookup;

  /** Lookup storing dyanmic strings to objects */
  typedef typename std::map<std::string, FactoryLookup<X> > MapType2;
  static MapType2 myDynamicInfo;

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

  /** Introduce new X to the rest of the system as dynamic loadable module. */
  static void
  introduceLazy(const std::string& name, const std::string& libname, const std::string& methodname)
  {
    // FIXME: Not checking double calls or anything at moment
    // Look for an existing record of this lazy loader...
    std::string key = libname + ":" + methodname;
    auto cur        = myDynamicInfo.find(key);

    if (cur != myDynamicInfo.end()) {
      auto& f = cur->second;
      f.alias.push_back(name);
    } else {
      auto z = FactoryLookup<X>(libname, methodname);
      z.alias.push_back(name);
      myDynamicInfo[key] = z;
    }
  }

  /** Get all currently loaded children for iteration */
  static MapType
  getAll()
  {
    return myLookup;
  }

  /** Force load dynamic modules, typically this is called by the
   * help system or debugging, say for help for the -o builders.
   * Errors aren't outputted since we're doing help  */
  static MapType2
  getAllDynamic()
  {
    for (auto& ele: myDynamicInfo) {
      auto& f = ele.second;
      for (auto& s:f.alias) {
        // Try to load once if not loaded already...
        if (!f.loaded) {
          std::shared_ptr<X> dynamicLoad = OS::loadDynamic<X>(f.module, f.methodname);
          f.stored = dynamicLoad;
          f.loaded = true; // Only try once
        }
        // ...and make sure object cached for the next call since it's not there
        if (f.stored != nullptr) {
          myLookup[s] = f.stored;
        }
      }
    }
    return myDynamicInfo;
  }

  /** Is this type known to the factory? */
  static bool
  exists(const std::string& name)
  {
    return (myLookup.count(name) != 0);
  }

  /** Return list of factory.  smart ptrs, so not a copy */
  static std::vector<std::shared_ptr<X> >
  toList(std::vector<std::string>& names)
  {
    std::vector<std::shared_ptr<X> > myList;

    for (auto ele: myLookup) {
      names.push_back(ele.first);
      myList.push_back(ele.second);
    }
    return myList;
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

    if (cur != myLookup.end()) { // Normally fairly quick...
      ret = cur->second;
    } else {
      // Hunt in the dynamic lazy loader list...
      for (auto& ele: myDynamicInfo) {
        auto& f = ele.second;
        for (auto& s:f.alias) {
          // if asked for key is in our alias list...
          if (s == name) {
            // Try to load once if not loaded already...
            if (!f.loaded) {
              LogInfo("Initial dynamic loading of library: " << f.module << "\n");
              std::shared_ptr<X> dynamicLoad = OS::loadDynamic<X>(f.module, f.methodname);
              if (dynamicLoad == nullptr) {
                LogSevere("...Unable to load dynamic library.\n");
              }
              f.stored = dynamicLoad;
              f.loaded = true; // Only try once
            }
            // ...and make sure object cached for the next call since it's not there
            if (f.stored != nullptr) {
              myLookup[name] = f.stored;
            }
            ret = f.stored;
            break;
          }
        }
      }
    }

    // Factory failed to find or load object
    // Lots of things optionally 'check' for a factory such as extension resolution, so
    // this can be kinda over talkative. Prefer checking nullptr yourself
    // after calling this and erroring if needed
    // if (ret == nullptr) {
    //  LogDebug("No instance available for " << info << " and '" << name
    //                                        << "' size: " << myLookup.size() << "\n");
    // }

    return (ret);
  } // get
};

/** Define the actual static member for the class */
template <class X>
typename Factory<X>::MapType Factory<X>::myLookup;
/** Define lookup from name to dynamic */
template <class X>
typename Factory<X>::MapType2 Factory<X>::myDynamicInfo;
}
