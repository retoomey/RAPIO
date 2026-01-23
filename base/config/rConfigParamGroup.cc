#include "rConfigParamGroup.h"
#include "rStrings.h"
#include "rError.h"
#include "rStreamIndex.h"

using namespace rapio;

std::vector<ConfigParamGroupi::indexInputInfo> ConfigParamGroupi::myIndexInputInfo;

std::vector<ConfigParamGroupI::productInputInfo> ConfigParamGroupI::myProductInputInfo;

std::vector<ConfigParamGroupo::outputInfo> ConfigParamGroupo::myWriters;

std::vector<ConfigParamGroupO::productOutputInfo> ConfigParamGroupO::myProductOutputInfo;

std::vector<ConfigParamGroupn::notifierInfo> ConfigParamGroupn::myNotifierInfo;

void
ConfigParamGroupi::process1(const std::string& param)
{
  // We don't actually create an index here...we wait until execute.
  fLogDebug("Adding source index:'{}'", param);

  std::string protocol, indexparams;

  // For the new xml=/home/code_index.xml protocol passing
  std::vector<std::string> pieces;

  // Macro ldm to the feedme binary by default for convenience
  if (param == "ldm") {
    protocol    = StreamIndex::STREAMINDEX;
    indexparams = "feedme%-f%TEXT";
  } else {
    // Split on = if able to get protocol, otherwise we'll try to guess
    Strings::split(param, '=', &pieces);
    const size_t aSize = pieces.size();
    if (aSize == 1) {
      protocol    = "";
      indexparams = pieces[0];
    } else if (aSize == 2) {
      protocol    = pieces[0];
      indexparams = pieces[1];
    } else {
      fLogSevere("Format of passed index '{}' is wrong, see help i", param);
      exit(1);
    }
  }

  myIndexInputInfo.push_back(indexInputInfo(protocol, indexparams));
}

void
ConfigParamGroupI::process1(const std::string& param)
{
  // inputs will be like this:
  // Reflectivity:00.50 VIL:00.50 etc.
  std::vector<std::string> pairs;

  Strings::splitWithoutEnds(param, ':', &pairs);

  // Add to our input info
  myProductInputInfo.push_back(productInputInfo(pairs[0], pairs.size() > 1 ? pairs[1] : ""));
}

void
ConfigParamGroupO::process1(const std::string& param)
{
  // Outputs will be like this:
  // Reflectivity:00.50 VIL:00.50 etc.
  // Reflectivity:00.50=MyRef1:00.50,  etc.
  // Reflectivity*=MyRef1*
  // I'm only supporting a single '*' match at moment...
  std::string fullproduct;
  std::string fullproductto;
  std::string productPattern   = "*"; // Match everything
  std::string subtypePattern   = "*";
  std::string toProductPattern = ""; // No conversion
  std::string toSubtypePattern = "";

  std::vector<std::string> pairs;

  Strings::splitWithoutEnds(param, '=', &pairs);

  if (pairs.size() > 1) { // X = Y form.  Remapping product/subtype names
    fullproduct   = pairs[0];
    fullproductto = pairs[1];

    pairs.clear();
    Strings::splitWithoutEnds(fullproductto, ':', &pairs);

    if (pairs.size() > 1) {
      toProductPattern = pairs[0];
      toSubtypePattern = pairs[1];
    } else {
      toProductPattern = pairs[0];
    }
  } else { // X form.  No remapping pattern...
    fullproduct = pairs[0];
  }

  // Split up the X full product pattern into pattern/subtype
  pairs.clear();
  Strings::splitWithoutEnds(fullproduct, ':', &pairs);

  if (pairs.size() > 1) {
    productPattern = pairs[0];
    subtypePattern = pairs[1];
  } else {
    productPattern = pairs[0];
    subtypePattern = "NOT FOUND";
  }

  //  fLogInfo("Product pattern is {}", productPattern);
  //  fLogInfo("Subtype pattern is {}", subtypePattern);
  //  fLogInfo("Product2 pattern is {}", toProductPattern);
  //  fLogInfo("Subtype2 pattern is {}", toSubtypePattern);

  // Make sure unique enough
  for (size_t i = 0; i < myProductOutputInfo.size(); i++) {
    if (productPattern == myProductOutputInfo[i].product) {
      fLogInfo("Already added output product with pattern '{}', ignoring..", productPattern);
      return;
    }
  }

  // Add the new output product declaration
  myProductOutputInfo.push_back(
    productOutputInfo(productPattern, subtypePattern, toProductPattern, toSubtypePattern));

  fLogDebug("Added output product pattern with key '{}'", param);
} // ConfigParamGroupO::process1

void
ConfigParamGroupo::process1(const std::string& param)
{
  std::vector<std::string> pair;

  Strings::splitWithoutEnds(param, '=', &pair);
  const size_t aSize = pair.size();

  if (aSize == 1) {
    // Use blank factory, the system will try to guess from
    // extension or the loaded datatype
    myWriters.push_back(outputInfo("", pair[0]));
  } else if (aSize == 2) {
    // Otherwise we have a factory attempt with this output info
    myWriters.push_back(outputInfo(pair[0], pair[1]));
  } else {
    fLogSevere("Can't understand your -o format for: '{}', ignoring.", param);
  }
}

void
ConfigParamGroupn::process1(const std::string& param)
{
  std::vector<std::string> pair;

  Strings::splitWithoutEnds(param, '=', &pair);

  std::string protocol;
  std::string params;
  const size_t aSize = pair.size();

  if (aSize == 1) {
    // Something like 'fml=' or 'test=', a type missing a parameter string
    if (Strings::endsWith(param, "=")) {
      protocol = pair[0];
      params   = "";
    } else {
      // Something like "/test", etc..try to read as location
      protocol = "fml";
      params   = pair[0];
    }
  } else if (aSize == 2) {
    protocol = pair[0];
    params   = pair[1];
  } else {
    fLogSevere("Format of passed notifier '{}' is wrong, see help n", param);
    exit(1);
  }

  myNotifierInfo.push_back(notifierInfo(protocol, params));
}

void
ConfigParamGroupn::process(const std::string& param)
{
  if (param == "disable") {
    fLogInfo("Notifiers disabled");
    myDisabled = true;
  } else {
    myDisabled = false;
    ConfigParamGroup::process(param);
  }
}

void
ConfigParamGroup::process(const std::string& param)
{
  // All the -i, -I, -O, -n, etc. all split first on ' '
  std::vector<std::string> pieces;

  Strings::splitWithoutEnds(param, ' ', &pieces);

  for (auto& p:pieces) {
    process1(p);
  }
}
