#pragma once

#include <string>
#include <iostream>
#include <vector>

using namespace std;

namespace rapio {
/** Stores product information required to convert WDSS-II NetCDF to
 * MRMS binary (QVS displayable) and back again.
 *
 * @author Carrie Langston
 *
 */
class ProductInfo
{
public:

  /** Build an empty product info (STL) */
  ProductInfo();

  /** Build a product info with fields set */
  ProductInfo(string vN, string vU, string vP, float vM, float vNC, int vS,
    string pD, string wN, string wU, float wM, float wNC) :
    varName(vN), varUnit(vU), varPrefix(vP), varMissing(vM),
    varNoCoverage(vNC), varScale(vS),
    productDir(pD), w2Name(wN), w2Unit(wU), w2Missing(wM), w2NoCoverage(wNC)
  { }

  /** Build a product info from another one */
  ProductInfo(const ProductInfo& pI);

  /** Destroy a product info */
  ~ProductInfo();

  /** Empty a product info */
  void
  clear();

  /** Undefined value for the Hmrg product table */
  static const float UNDEFINED;

  // Table columns

  string varName;
  string varUnit;
  string varPrefix;
  float varMissing;
  float varNoCoverage;
  int varScale;
  string productDir;
  string w2Name;
  string w2Unit;
  float w2Missing;
  float w2NoCoverage;
};

/** Store a group of ProductInfos */
class ProductInfoSet {
public:

  /** Create a product info set */
  ProductInfoSet(){ }

  /** Store set of product infos */
  void
  readConfigFile();

  /** Get the product info based on fields */
  ProductInfo *
  getProductInfo(const std::string& varName, const std::string& units);

  /** Give back W2 info based on passed in HMRG */
  bool
  HmrgToW2Name(const std::string& varName,
    std::string                 & outW2Name);

  /** Print the table for debugging */
  void
  dump();

protected:

  /** Row storage in our table */
  std::vector<ProductInfo> myProductInfos;
};
}
