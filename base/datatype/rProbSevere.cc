#include "rProbSevere.h"
#include "rError.h"
#include "rStrings.h"

using namespace rapio;

ProbSevere::ProbSevere(std::shared_ptr<PTreeData> genericTree)
{
  // Establish our specialized identity regardless of how we were created
  setDataType("ProbSevere");
  setTypeName("ProbSevere");

  // If we were handed a tree, promote it
  if (genericTree) {
    // FIXME: 'Could' have a helper in PTreeData, etc.
    myAttributes  = genericTree->getGlobalAttributes();
    myTime        = genericTree->getTime();
    myLocation    = genericTree->getLocation();
    myReadFactory = genericTree->getReadFactory();
    myRoot        = genericTree->getTree();

    // Extract the Version
    std::string productStr = myRoot->get<std::string>("product", "");
    std::vector<std::string> pieces;

    // splitOnFirst separates "ProbSevere" from "3.0"
    if (Strings::splitOnFirst(productStr, " ", pieces)) {
      myVersion = pieces[1];
    } else {
      myVersion = "1.0"; // Fallback if it's just "ProbSevere" without a version
    }

    parseFeatures();
  }
}

void
ProbSevere::parseFeatures()
{
  myFeatures.clear();
  auto root = getTree();

  // 1. Extract and set the Time
  // Adjust the JSON key ("validTime") and strftime format to match the actual file.
  // E.g., for "20261006_162600" use "%Y%m%d_%H%M%S"
  // E.g., for "2026-10-06 16:26:00Z" use "%Y-%m-%d %H:%M:%SZ"
  std::string timeStr = root->get<std::string>("validTime", "");

  if (!timeStr.empty()) {
    try {
      setTime(Time(timeStr, "%Y%m%d_%H%M%S UTC"));
    } catch (const std::exception& e) {
      fLogSevere("Failed to parse ProbSevere time: {}", e.what());
    }
  }

  // 2. Set the Location
  // ProbSevere files typically cover the whole US, so you can set a generic CONUS
  // center, or extract bounding box coordinates if they exist in the JSON.
  setLocation(LLH(39.8283, -98.5795, 0.0));

  // 3. Continue parsing your spatial features...
  auto featuresNode = root->getChildOptional("features");

  if (!featuresNode) { return; }

  auto features = featuresNode->getChildren("item");

  for (const auto& feature : features) {
    ProbSevereFeature psDet;

    auto properties = feature.getChildOptional("properties");
    if (properties) {
      psDet.id = properties->get<int>("ID", -1);
      psDet.probSevereVal = properties->get<float>("PS", 0.0f);

      // Extract environmental storm motion vectors and convert from knots to m/s
      float motionEast  = properties->get<float>("MOTION_EAST", 0.0f);
      float motionSouth = properties->get<float>("MOTION_SOUTH", 0.0f);

      psDet.u_motion = motionEast / 1.944f;
      psDet.v_motion = -motionSouth / 1.944f;
    }

    auto geometry = feature.getChildOptional("geometry");
    if (geometry) {
      auto coordinates = geometry->getChildOptional("coordinates");
      if (coordinates) {
        auto rings = coordinates->getChildren("item");
        if (!rings.empty()) {
          auto points = rings[0].getChildren("item");
          for (const auto& point : points) {
            auto coords = point.getChildren("item");
            if (coords.size() >= 2) {
              double lon = coords[0].get<double>(0.0);
              double lat = coords[1].get<double>(0.0);
              psDet.polygon.push_back(LL(lat, lon));
            }
          }
        }
      }
    }

    if (!psDet.polygon.empty()) {
      myFeatures.push_back(psDet);
    }
  }
} // ProbSevere::parseFeatures
