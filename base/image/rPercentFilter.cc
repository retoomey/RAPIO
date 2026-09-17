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
PercentFilter::parseOptions(const std::vector<std::string>& parts)
{
  float cutoff  = myPercentile * 100.0f;
  float minFill = 0.33f;

  try {
    if (parts.size() > 1) { cutoff = std::stof(parts[1]); }
    if (parts.size() > 2) { myHalfX = std::stoi(parts[2]); }
    if (parts.size() > 3) { minFill = std::stof(parts[3]); }
    myHalfY = myHalfX;
    if (parts.size() > 4) { myHalfY = std::stoi(parts[4]); }

    myPercentile = cutoff / 100.0f;
    int totalWindowSize = (2 * myHalfX + 1) * (2 * myHalfY + 1);
    myMinFillCount = static_cast<int>(minFill * totalWindowSize + 0.5f);
  } catch (const std::exception& e) {
    fLogSevere("PercentFilter param error: {}", e.what());
    return false;
  }
  return true;

  #if 0
  // We need temporary variables for things that require math before assignment
  float cutoff  = myPercentile * 100.0f; // Default from header
  float minFill = 0.33f;

  getParam(parts, 1, cutoff);
  getParam(parts, 2, halfX);
  getParam(parts, 3, minFill);

  // Derived default: halfY defaults to whatever halfX is NOW.
  halfY = halfX;
  getParam(parts, 4, halfY); // Overwrite only if the user specifically provided it

  // Apply math to final state
  myPercentile   = cutoff / 100.0f;
  myMinFillCount = static_cast<int>(minFill * (2 * halfX + 1) * (2 * halfY + 1) + 0.5f);

  return true;

  #endif // if 0
} // PercentFilter::parseOptions

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

  if (myXBoundary == Boundary::Wrap) {
    if (myYBoundary == Boundary::Wrap) {
      applyFilter<BoundWrap, BoundWrap>(src, dst);
    } else if (myYBoundary == Boundary::Clamp) { applyFilter<BoundWrap, BoundClamp>(src, dst); } else {
      applyFilter<BoundWrap, BoundNone>(src, dst);
    }
  } else if (myXBoundary == Boundary::Clamp) {
    if (myYBoundary == Boundary::Wrap) {
      applyFilter<BoundClamp, BoundWrap>(src, dst);
    } else if (myYBoundary == Boundary::Clamp) { applyFilter<BoundClamp, BoundClamp>(src, dst); } else {
      applyFilter<BoundClamp, BoundNone>(src, dst);
    }
  } else {
    if (myYBoundary == Boundary::Wrap) {
      applyFilter<BoundNone, BoundWrap>(src, dst);
    } else if (myYBoundary == Boundary::Clamp) { applyFilter<BoundNone, BoundClamp>(src, dst); } else {
      applyFilter<BoundNone, BoundNone>(src, dst);
    }
  }
}
