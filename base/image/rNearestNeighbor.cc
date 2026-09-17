#include <rNearestNeighbor.h>
#include <rFactory.h>

using namespace rapio;
using namespace std;

void
NearestNeighbor::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<NearestNeighbor>();
  Factory<ArraySampler>::introduce("nearest", newOne);
};

std::string
NearestNeighbor::getHelpString()
{
  return " -- Nearest Neighbor.";
}
