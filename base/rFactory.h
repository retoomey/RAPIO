#pragma once

#include <map>
#include <string>
#include <rError.h>
#include <rOS.h>

#include <vector>
#include <memory>
#include <mutex>

namespace rapio {
/**
 * Factory create a name to object lookup based on type.
 * It has the ability to lazy load from dynamic libraries
 * on demand to save runtime memory.
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Stores created objects by type for lookup by name.
 */
template <class X> class Factory {
private:

  /** Simple private lookup for late loaded dynamic factories */
  template <class Z> class FactoryLookup {
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
  using MapType = std::map<std::string, std::shared_ptr<X> >;
  static MapType myLookup;

  /** Lookup storing dyanmic strings to objects */
  using MapType2 = std::map<std::string, FactoryLookup<X> >;
  static MapType2 myDynamicInfo;

  /** Mutex for thread safety */
  static std::mutex myMutex;

  /** Private Helper: Ensure the dynamic module is loaded.
   * If loaded successfully, it registers ALL aliases into the main lookup.
   * NOTE: This method assumes myMutex is ALREADY LOCKED by the caller.
   */
  static void
  ensureLoaded(FactoryLookup<X>& f, bool verbose)
  {
    if (!f.loaded) {
      if (verbose) {
        fLogInfo("Initial dynamic loading of library: {}", f.module);
      }

      std::shared_ptr<X> dynamicLoad = OS::loadDynamic<X>(f.module, f.methodname);

      f.stored = dynamicLoad;
      f.loaded = true; // Only try once
      if (dynamicLoad != nullptr) {
        // Update alias
        for (const auto& aliasName : f.alias) {
          myLookup[aliasName] = dynamicLoad;
        }
      } else {
        if (verbose) {
          fLogSevere("...Unable to load dynamic library.");
        }
      }
    }
  }

public:

  /** Introduce new X to the rest of the system. */
  static void
  introduce(const std::string& name, std::shared_ptr<X> instance)
  {
    std::lock_guard<std::mutex> lock(myMutex);

    if (instance != nullptr) {
      myLookup[name] = instance;
    } else {
      fLogSevere("invalid instance pointer for '{}'", name);
    }
  }

  /** Introduce new X to the rest of the system as dynamic loadable module. */
  static void
  introduceLazy(const std::string& name, const std::string& libname, const std::string& methodname)
  {
    // FIXME: Not checking double calls or anything at moment
    // Look for an existing record of this lazy loader...
    std::lock_guard<std::mutex> lock(myMutex);
    std::string key = libname + ":" + methodname;
    auto cur        = myDynamicInfo.find(key);

    if (cur != myDynamicInfo.end()) {
      auto& f = cur->second;
      f.alias.push_back(name);

      // Edge Case: If the module is ALREADY loaded when we add a new alias,
      // we must manually add this new alias to myLookup now, because
      // ensureLoaded won't run again for this module.
      if (f.loaded && f.stored) {
        myLookup[name] = f.stored;
      }
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
    std::lock_guard<std::mutex> lock(myMutex);

    return myLookup;
  }

  /** Force load dynamic modules, typically this is called by the
   * help system or debugging, say for help for the -o builders.
   * Errors aren't outputted since we're doing help  */
  static MapType2
  getAllDynamic()
  {
    std::lock_guard<std::mutex> lock(myMutex);

    for (auto& ele: myDynamicInfo) {
      ensureLoaded(ele.second, false);
    }
    return myDynamicInfo;
  }

  /** Is this type known to the factory? */
  static bool
  exists(const std::string& name)
  {
    std::lock_guard<std::mutex> lock(myMutex);

    // 1. Check if it is already created/loaded
    if (myLookup.count(name) != 0) { return true; }

    // 2. Check if it is a known lazy alias (iterate because keys are lib paths)
    for (const auto& pair : myDynamicInfo) {
      const auto& factoryLookup = pair.second;
      for (const auto& alias : factoryLookup.alias) {
        if (alias == name) { return true; }
      }
    }

    return false;
  }

  /** Return list of factory.  smart ptrs, so not a copy */
  static std::vector<std::shared_ptr<X> >
  toList(std::vector<std::string>& names)
  {
    std::lock_guard<std::mutex> lock(myMutex);
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
    std::lock_guard<std::mutex> lock(myMutex);

    typename MapType::const_iterator cur = myLookup.find(name);

    if (cur != myLookup.end()) { // Normally fairly quick...
      return cur->second;
    }

    // Hunt in the dynamic lazy loader list...
    for (auto& ele: myDynamicInfo) {
      auto& f = ele.second;
      for (auto& s:f.alias) {
        // if asked for key is in our alias list...
        if (s == name) {
          ensureLoaded(f, true);
          return f.stored;
        }
      }
    }

    // Factory failed to find or load object
    // Lots of things optionally 'check' for a factory such as extension resolution, so
    // this can be kinda over talkative. Prefer checking nullptr yourself
    // after calling this and erroring if needed
    // fLogDebug("No instance available for {} and '{}' size: {}", info, name, myLookup.size());

    return (nullptr);
  } // get
};

/** Define the actual static member for the class */
template <class X>
typename Factory<X>::MapType Factory<X>::myLookup;
/** Define lookup from name to dynamic */
template <class X>
typename Factory<X>::MapType2 Factory<X>::myDynamicInfo;
/** Define the static mutex */
template <class X>
std::mutex Factory<X>::myMutex;
}
