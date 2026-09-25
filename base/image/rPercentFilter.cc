#include <rPercentFilter.h>
#include <rFactory.h>
#include <rError.h>
#include <algorithm>

using namespace rapio;

void
PercentFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOne = std::make_shared<PercentFilter>();
  Factory<ArrayFilter>::introduce("percent", newOne);
}

bool
PercentFilter::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }

  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);

  float cutoff  = myPercentile * 100.0f;
  float minFill = 0.33f;

  try {
    if (parts.size() > 0) { cutoff = std::stof(parts[0]); }
    if (parts.size() > 1) { myHalfX = std::stoi(parts[1]); }
    if (parts.size() > 2) { minFill = std::stof(parts[2]); }
    myHalfY = myHalfX;
    if (parts.size() > 3) { myHalfY = std::stoi(parts[3]); }

    myPercentile = cutoff / 100.0f;
    int totalWindowSize = (2 * myHalfX + 1) * (2 * myHalfY + 1);
    myMinFillCount = static_cast<int>(minFill * totalWindowSize + 0.5f);
  } catch (const std::exception& e) {
    fLogSevere("PercentFilter param error: {}", e.what());
    return false;
  }
  return true;
}

std::string
PercentFilter::getHelpString()
{
  return "{percent=50}:{halfSizeX=5}:{minFill=0.33}:{halfSizeY=5} -- Nth percentile filter.";
}

// ---------------------------------------------------------
// The Templated Inner Loop
// ---------------------------------------------------------
template <typename BndX, typename BndY>
void
PercentFilter::applyFilter(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                          dst)
{
  auto& srcData = src->ref();
  auto& dstData = dst->ref();
  int width     = src->getX();
  int height    = src->getY();

  std::vector<float> neighbors;

  neighbors.reserve((2 * myHalfX + 1) * (2 * myHalfY + 1));

  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      neighbors.clear();

      for (int m = i - myHalfX; m <= i + myHalfX; ++m) {
        int memX = m;
        if (!BndX::resolve(memX, width)) {
          continue; // Resolves boundary immediately
        }
        for (int n = j - myHalfY; n <= j + myHalfY; ++n) {
          int memY = n;
          if (!BndY::resolve(memY, height)) {
            continue; // Resolves boundary immediately
          }
          float val = srcData[memX][memY];
          if (val != Constants::MissingData) {
            neighbors.push_back(val);
          }
        }
      }

      if (neighbors.size() >= static_cast<size_t>(myMinFillCount)) {
        size_t nIndex = static_cast<size_t>(neighbors.size() * myPercentile);
        if (nIndex >= neighbors.size()) { nIndex = neighbors.size() - 1; }
        std::nth_element(neighbors.begin(), neighbors.begin() + nIndex, neighbors.end());
        dstData[i][j] = neighbors[nIndex];
      } else {
        dstData[i][j] = Constants::MissingData;
      }
    }
  }
} // PercentFilter::applyFilter

// ---------------------------------------------------------
// The Runtime Dispatcher
// ---------------------------------------------------------
void
PercentFilter::process(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                      dst)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst);
}
