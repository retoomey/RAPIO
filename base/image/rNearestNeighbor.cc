#include <rNearestNeighbor.h>
#include <rFactory.h>

using namespace rapio;
using namespace std;

void
NearestNeighbor::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<NearestNeighbor>();
  Factory<ArraySampler>::introduce("neareset", newOne);
};

bool
NearestNeighbor::sampleAt(float inI, float inJ, float& out)
{
  // for nearest, round to closest cell hit
  // lround returns a long int directly, avoiding float->int cast
  int i = std::lround(inI);

  // if ((i < 0) || (i >= myMaxI)) {
  if (!resolveX(i)) {
    return false;
  }
  int j = std::lround(inJ);

  // if ((j < 0) || (j >= myMaxJ)) {
  if (!resolveY(j)) {
    return false;
  }
  out = (*myRefIn)[i][j];
  return true;
}

std::string
NearestNeighbor::getHelpString()
{
  return " -- Nearest Neighbor.";
}
