#include <rBilinear.h>
#include <rError.h>
#include <rFactory.h>

using namespace rapio;
using namespace std;

void
Bilinear::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<Bilinear>();
  Factory<ArraySampler>::introduce("bilinear", newOne);
};

bool
Bilinear::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }

  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);

  if (parts.size() < 2) {
    return true;
  }
  try {
    myWidth  = std::stoul(parts[0]);
    myHeight = std::stoul(parts[1]);

    if (parts.size() > 2) {
      fLogSevere("Warning: 'bilinear' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 2);
    }
  } catch (const std::exception& e) {
    fLogSevere("Bilinear param error: {}", e.what());
    return false;
  }
  return true;
}

std::string
Bilinear::getHelpString()
{
  return "{width=3}:{height=3} -- Bilinear interpolation.";
}
