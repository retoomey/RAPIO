#pragma once
#include <rURL.h>
#include <rError.h>

namespace rapio {
/** An API for specifying how to output a DataType.
 * Default system flags and abilities will become API get/set
 * in order to avoid magic strings and mistakes/unknown
 * general abilities.
 *
 * Modules will still have the ability to set special
 * flags by string. Recommend constants set in the modules,
 * for example netcdf flags from rapiosettings.xml.
 *
 * @author Robert Toomey
 */
class IOConfig {
public:

  /** Sparse mode.  Guess tries to avoid full scanning,
   * Hard scans for maximum IO savings */
  enum class SparseMode {
    None,  // Never sparse
    Guess, // Partial scan guess
    Hard,  // Full scan of data
    Force  // Always sparse, even if LARGER.  For debugging
  };

  static constexpr const char * SPARSE_MODE_NONE  = "none";
  static constexpr const char * SPARSE_MODE_GUESS = "guess";
  static constexpr const char * SPARSE_MODE_HARD  = "hard";
  static constexpr const char * SPARSE_MODE_FORCE = "force";
  static constexpr const char * KEY_SPARSE_MODE   = "sparse_mode";
  static constexpr const char * KEY_SPARSE_THRESH = "sparse_threshold";

  IOConfig() = default;

  /** Return command line parameter string as a URL */
  URL
  getParamURL() const
  {
    return URL(myParams);
  }

  void
  setParams(std::string_view p)
  {
    myParams = p;
  }

  /** Set a special string by key.  Will be used by dynamic modules
   * to store non-standard custom values */
  IOConfig&
  set(const std::string& key, std::string_view value)
  {
    myLookup[key] = value;
    return *this;
  }

  /** Check if a key exists */
  bool
  has(const std::string& key) const
  {
    return myLookup.find(key) != myLookup.end();
  }

  /** Get a key with a safe default */
  std::string
  get(const std::string& key, std::string_view defaultValue = "") const
  {
    auto it = myLookup.find(key);

    if (it != myLookup.end()) {
      return it->second;
    }
    return std::string(defaultValue);
  }

  /** Return map for backward compatibility for now */
  const std::map<std::string, std::string>&
  toMap() const
  {
    return myLookup;
  }

  /** Temp legacy code compatibility if needed */
  std::map<std::string, std::string>&
  getMapRef()
  {
    return myLookup;
  }

  /** Temp legacy code hack */
  void
  setMap(const std::map<std::string, std::string>& in)
  {
    myLookup = in;
  }

  // Sparse controls ------------------------------------

  /** Set the sparse mode */
  void
  setSparseMode(SparseMode mode)
  {
    switch (mode) {
        case SparseMode::None:  set(KEY_SPARSE_MODE, SPARSE_MODE_NONE);
          break;
        case SparseMode::Guess: set(KEY_SPARSE_MODE, SPARSE_MODE_GUESS);
          break;
        case SparseMode::Hard:  set(KEY_SPARSE_MODE, SPARSE_MODE_HARD);
          break;
        case SparseMode::Force: set(KEY_SPARSE_MODE, SPARSE_MODE_FORCE);
          break;
    }
  }

  /** Get the current sparse mode */
  SparseMode
  getSparseMode() const
  {
    std::string modeStr = get(KEY_SPARSE_MODE);

    if (modeStr.empty()) { return getGlobalSparseMode(); }

    for (char& c : modeStr) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

    if (modeStr == SPARSE_MODE_NONE) { return SparseMode::None; }
    if (modeStr == SPARSE_MODE_GUESS) { return SparseMode::Guess; }
    if (modeStr == SPARSE_MODE_HARD) { return SparseMode::Hard; }
    if (modeStr == SPARSE_MODE_FORCE) { return SparseMode::Force; }

    return getGlobalSparseMode();
  }

  /** Set the threshold value used for guess or hard sparse mode */
  void
  setSparseThreshold(float threshold)
  {
    // Clamp threshold between 0.0 and 1.0
    float clamped = std::max(0.0f, std::min(1.0f, threshold));

    set(KEY_SPARSE_THRESH, std::to_string(clamped));
    fLogInfo("Sprase threshhold set to {}", clamped);
  }

  /** Get the threshold value used for guess or hard sparse mode */
  float
  getSparseThreshold() const
  {
    auto it = myLookup.find(KEY_SPARSE_THRESH);

    if (it != myLookup.end()) {
      try {
        return std::stof(it->second);
      } catch (...) { }
    }
    return getGlobalSparseThreshold();
  }

  /** Called by rConfigDataType only to set system default by string */
  static void
  setGlobalSparseDefaults(std::string_view modeStr, float threshold);

  /** Get global default sparse mode from configuration */
  static SparseMode
  getGlobalSparseMode();

  /** Get global default threshold from configuration */
  static float
  getGlobalSparseThreshold();

  /** Turn off unsparsing of incoming data. Doing this could crash
   * access that assumes normal arrays.  Used only by programs like
   * rcopy to avoid unsparse/sparse thrashing on copying files */
  static void setGlobalDoUnsparse(bool flag){ ourDoUnsparse = flag; }

  /** Get the current no unsparse state */
  static bool getGlobalDoUnsparse(){ return ourDoUnsparse; }

  // End Sparse controls ------------------------------------
private:

  /** Params from command line */
  std::string myParams;

  /** Generic storage */
  std::map<std::string, std::string> myLookup;

  /** Global sparse mode */
  static SparseMode ourDefaultSparseMode;

  /** Global sparse threshold */
  static float ourDefaultSparseThreshold;

  /** Global turn off unsparse on read (used by rcopy) */
  static bool ourDoUnsparse;
};
}
