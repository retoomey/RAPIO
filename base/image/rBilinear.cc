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
Bilinear::parseOptions(const std::vector<std::string>& parts)
{
  if (parts.size() < 3) {
    // Use defaults if not enough params
    return true;
  }

  try {
    // Handle our params
    myWidth  = std::stoul(parts[1]);
    myHeight = std::stoul(parts[2]);

    if (parts.size() > 3) {
      fLogSevere("Warning: 'bilinear' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 3);
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
