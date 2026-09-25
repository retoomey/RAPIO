#include <rCressman.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;
using namespace std;

void
Cressman::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<Cressman>();
  Factory<ArraySampler>::introduce("cressman", newOne);
};

bool
Cressman::parseOptions(const std::string& params)
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
    fLogSevere("Legacy cressman width/height {}, {}", myWidth, myHeight);

    if (parts.size() > 2) {
      fLogSevere("Warning: 'cressman' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 2);
    }
  } catch (const std::exception& e) {
    fLogSevere("Cressman param error: {}", e.what());
    return false;
  }
  return true;
}

std::string
Cressman::getHelpString()
{
  return "{width=3}:{height=3} -- Cressman interpolation.";
}
