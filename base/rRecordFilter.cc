#include "rRecordFilter.h"
#include "rRAPIOAlgorithm.h"
#include "rStrings.h"

using namespace rapio;

bool
AlgRecordFilter::wanted(const Record& rec)
{
  // sel is a set of strings like this: time datatype subtype
  // myType is a set of strings like  :      datatype subtype
  //                              or  :      datatype
  // i.e. we need to match sel[1] with myType[0]
  // and maybe sel[2] with myType[1]
  std::vector<std::string> sel = rec.getSelections();

  auto info = ConfigParamGroupI::getProductInputInfo();
  for (auto& p:info) {
    // If no subtype, just match the product pattern. For example, something
    // like
    // Vel* will match Velocity
    std::string star;
    bool matchProduct = false;
    bool matchSubtype = false;

    // -----------------------------------------------------------
    // Match product pattern if selections have it
    if (sel.size() > 1) {
      // Not sure we need this or why..it was in the old code though...
      if ((sel.back() != "vol") && (sel.back() != "all")) {
        matchProduct = Strings::matchPattern(p.name, sel[1], star);
      }
    }

    // -----------------------------------------------------------
    // Match subtype pattern if we have one (and product already matched)
    if (p.subtype == "") {
      matchSubtype = true; // empty is pretty much '*'
    } else {
      if (matchProduct && (sel.size() > 2)) {
        matchSubtype = Strings::matchPattern(p.subtype, sel[2], star);
      }
    }

    // Return on matched product/subtype
    if (matchProduct && matchSubtype) {
      //    matchedPattern = p.name;
      return (true);
    }
  }
  // matchedPattern = "";
  return (false);
} // AlgRecordFilter::wanted
