#include "rHmrgProductInfo.h"

#include "rConfig.h"
#include "rIOURL.h"

using namespace rapio;
using namespace std;

const float ProductInfo::UNDEFINED = 12345.0;

void
ProductInfoSet::readConfigFile()
{
  // On start up attempt to read the conversion file.  We can still
  // run without it kinda...
  URL url      = Config::getConfigFile("W2_TO_MRMS_PRODUCTS.dat");
  bool success = false;

  if (!url.empty()) {
    std::vector<char> buf;
    if (IOURL::read(url, buf) > 0) {
      buf.push_back('\0');
      std::istringstream is(&buf.front());
      std::string s;
      // Each line
      success = true;
      bool header = true;
      while (getline(is, s, '\n')) {
        if (header) { header = false; continue; }
        if (s.empty()) { continue; }
        if (s[0] == '#') { continue; } // comments on first column

        std::string hdrDummy, comment, ref_line;
        std::string varName, varUnit, varPrefix, w2Name, w2Unit;
        std::string productDir, temp;
        float varMissing, w2Missing, varNoCoverage, w2NoCoverage;
        int varScale;

        try{
          std::istringstream line{ s };

          // Just snag each of the unique fields
          line >> varName;
          line >> varUnit;
          line >> varPrefix;
          line >> productDir;
          line >> temp;
          varMissing = (temp == "none") ? ProductInfo::UNDEFINED : atof(temp.c_str());
          line >> temp;
          varNoCoverage = (temp == "none") ? ProductInfo::UNDEFINED : atof(temp.c_str());
          line >> varScale;
          line >> w2Name;
          line >> w2Unit;
          line >> w2Missing;
          line >> w2NoCoverage;

          myProductInfos.push_back(ProductInfo(varName, varUnit, varPrefix,
            varMissing, varNoCoverage, varScale, productDir, w2Name, w2Unit, w2Missing, w2NoCoverage));
        }catch (const std::exception& e) {
          fLogSevere("Error readingHMRG product info file: {}", e.what());
          success = false;
        }
      }
    }
  }
  if (success) {
    fLogInfo("HMRG module: Successful reading of W2/HMRG product info table.");
  } else {
    fLogSevere("HMRG module: Errors reading W2/HMRG product info module.");
  }
} // ProductInfoSet::readConfigFile

ProductInfo *
ProductInfoSet::getProductInfo(const std::string& w2Name, const std::string& w2Units)
{
  ProductInfo * pi;

  for (size_t i = 0; i < myProductInfos.size(); ++i) {
    // FIXME: Should we try to match more than name and units?
    // I could see cases with strange missing/unavailable values to deal with.
    // For now this 'should' get like 99% of things
    if (myProductInfos[i].w2Name == w2Name) {
      if (myProductInfos[i].w2Unit == w2Units) {
        return &myProductInfos[i];
      }
    }
  }
  return nullptr;
}

bool
ProductInfoSet::HmrgToW2Name(const std::string& varName,
  std::string                                 & outW2Name)
{
  bool success = false;

  for (auto& p:myProductInfos) {
    // Ok for moment, first match will be it?  The table isn't that great
    // FIXME: units, etc. maybe?  For now I just want to enforce color maps for
    // tiling properly
    if (p.varName == varName) {
      outW2Name = p.w2Name;
      success   = true;
      break;
    }
  }
  return success;
}

ProductInfo::ProductInfo()
{
  varName.clear();
  varUnit.clear();
  varPrefix.clear();
  varMissing    = UNDEFINED;
  varNoCoverage = UNDEFINED;
  varScale      = (int) UNDEFINED;

  productDir.clear();

  w2Name.clear();
  w2Unit.clear();
  w2Missing    = UNDEFINED;
  w2NoCoverage = UNDEFINED;
}

void
ProductInfoSet::dump()
{
  fLogInfo("There are {} product infos stored:", myProductInfos.size());
  for (auto& p:myProductInfos) {
    std::cout << p.varName << ", ";
    std::cout << p.varUnit << ", ";
    std::cout << p.varPrefix << ", ";
    std::cout << p.productDir << ", ";
    std::cout << p.varMissing << ", ";
    std::cout << p.varNoCoverage << ", ";
    std::cout << p.varScale << ", ";
    std::cout << p.w2Name << ", ";
    std::cout << p.w2Unit << ", ";
    std::cout << p.w2Missing << ", ";
    std::cout << p.w2NoCoverage << ", ";
    std::cout << "\n";
  }
}

ProductInfo::ProductInfo(const ProductInfo& pI)
{
  varName       = pI.varName;
  varUnit       = pI.varUnit;
  varPrefix     = pI.varPrefix;
  varMissing    = pI.varMissing;
  varNoCoverage = pI.varNoCoverage;
  varScale      = pI.varScale;

  productDir = pI.productDir;

  w2Name       = pI.w2Name;
  w2Unit       = pI.w2Unit;
  w2Missing    = pI.w2Missing;
  w2NoCoverage = pI.w2NoCoverage;
}

ProductInfo::~ProductInfo(){ }

void
ProductInfo::clear()
{
  varName.clear();
  varUnit.clear();
  varPrefix.clear();
  varMissing    = UNDEFINED;
  varNoCoverage = UNDEFINED;
  varScale      = (int) UNDEFINED;

  productDir.clear();

  w2Name.clear();
  w2Unit.clear();
  w2Missing    = UNDEFINED;
  w2NoCoverage = UNDEFINED;
}
