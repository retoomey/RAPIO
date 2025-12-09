#pragma once

#include <rConfig.h>

#include <unordered_map>
#include <string>

namespace rapio {
/** Holder for a NIDSInfo entry.
 * Level III data isn't completely self-describing, so
 * any extra information needed from product codes will be
 * stored into a lookup table.
 *
 * @author Lulin Song, Robert Toomey
 */
class NIDSInfo : public Data {
  friend class ConfigNIDSInfo;
public:

  /** STL Construct an invalid NIDSInfo */
  NIDSInfo() : myCode(-1), myDecode(1), myMin(-1), myIncrease(-1){ }

  /** Construct a NIDSInfo */
  NIDSInfo(
    int              aCode,
    const std::string& aType,
    const std::string& aResolution,
    int              aRange,
    int              aLevel,
    const std::string& units,
    const std::string& msg,
    bool             compress,
    int              decode,
    int              min,
    int              increase) :
    myCode(aCode), myType(aType),
    myResolution(aResolution), myRange(aRange),
    myLevel(aLevel), myUnits(units),
    myMsgFormat(msg), myCompression(compress),
    myDecode(decode),
    myMin(min), myIncrease(increase){ }

  /** Is this NIDS info valid? */
  bool
  isValid() const { return myCode != -1; }

  /** Get product code of this info */
  int
  getCode() const { return myCode; }

  /** Get type */
  std::string
  getType() const { return myType; }

  /** Get product name */
  std::string
  getProductName() const;

  /** Get resolution */
  std::string
  getResolution() const { return myResolution; }

  /** Get range */
  int
  getRange() const { return myRange; }

  /** Get level */
  int
  getLevel() const { return myLevel; }

  /** Get units */
  std::string
  getUnits() const { return myUnits; }

  /** Get msg format */
  std::string
  getMsgFormat() const { return myMsgFormat; }

  /** Get compression */
  bool
  getChkCompression() const { return myCompression; }

  /** Do we have threshold scale information?  */
  bool
  haveScaledThresholds() const { return ((myMin != -1) && (myIncrease != -1)); }

  /** Get decode method number or default */
  int
  getDecode() const { return myDecode; }

  /** Get min or -1 if no scaling */
  int
  getMin() const { return myMin; }

  /** Get increase or -1 if no scaling */
  int
  getIncrease() const { return myIncrease; }

  /** Is this a null product? */
  bool
  isNullProduct() const
  {
    return ( (myCode == 31) || (myCode == 169) || (myCode == 170) ||
           (myCode == 171) || (myCode == 172) || (myCode == 173) ||
           (myCode == 175) );
  }

  /** Does data need the thresholds decoded? */
  bool
  needsDecodedThresholds() const
  {
    // Currently these three don't need decoded thresholds
    return !((myCode == 134) || (myCode == 135) || (myCode == 176));
  }

  // RadialSet info

  /** Get gate width in meters */
  LengthMs
  getGateWidthMeters() const;

  /** Operator << write out a NIDSInfo */
  friend std::ostream&
  operator << (std::ostream& os, const NIDSInfo&);

private:

  int myCode;
  std::string myType;
  std::string myResolution;
  int myRange;
  int myLevel;
  std::string myUnits;
  std::string myMsgFormat;
  bool myCompression;
  int myDecode;
  // Used for scaled products
  int myMin;
  int myIncrease;
};

/** Reads a static database for storing NIDS information */
class ConfigNIDSInfo : public ConfigType {
public:

  /** Call this to free the database RAM if no longer needed,
   * the database will be reread if called again */
  static void
  releaseDatabase()
  {
    myNIDSInfos.clear();
    myReadSettings = false;
  }

  /** Check and load settings */
  static void
  loadSettings();

  /** Do we have a particular product code? */
  static int
  haveCode(int code);

  /** Get NIDSInfo given a code, or empty one */
  static NIDSInfo
  getNIDSInfo(int code);

  /** Path for our configuration file */
  static const std::string ConfigNIDSInfoXML;

  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData> d);

private:

  /** Have we read settings yet? */
  static bool myReadSettings;

  /** Map from codes to NID Infos */
  static std::unordered_map<int, NIDSInfo> myNIDSInfos;
};

/** Output a NIDSInfo */
std::ostream&
operator << (std::ostream&,
  const rapio::NIDSInfo&);
}
