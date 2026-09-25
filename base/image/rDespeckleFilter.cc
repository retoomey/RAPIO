#include <rDespeckleFilter.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;

void
DespeckleFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOne = std::make_shared<DespeckleFilter>();
  Factory<ArrayFilter>::introduce("despeckle", newOne);
}

std::string
DespeckleFilter::getHelpString()
{
  return
    "{halfSizeX=1}:{halfSizeY=1}:{minFill=0.33} -- Removes speckles without smoothing. Note: minFill is calculated against a single quadrant (halfSizeX * halfSizeY), not the full kernel.";
}

bool
DespeckleFilter::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }

  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);
  try {
    if (parts.size() > 0) { myHalfSizeX = std::stoi(parts[0]); }
    if (parts.size() > 1) { myHalfSizeY = std::stoi(parts[1]); }
    if (parts.size() > 2) { myMinFillFrac = std::stof(parts[2]); }

    // Restoring the exact legacy calculation!
    // This evaluates a fraction of a single quadrant (hsx * hsy) rather than the full window.
    myMinFillCount = static_cast<int>(myMinFillFrac * myHalfSizeX * myHalfSizeY + 0.5f);
    if (myMinFillCount < 0) { myMinFillCount = 0; }
  } catch (const std::exception& e) {
    fLogSevere("DespeckleFilter param error: {}", e.what());
    return false;
  }
  return true;
}

template <typename BndX, typename BndY>
void
DespeckleFilter::applyFilter(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                            dst)
{
  auto& srcData = src->ref();
  auto& dstData = dst->ref();
  int width     = src->getX();
  int height    = src->getY();

  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      float val = srcData[i][j];
      if (!Constants::isGood(val)) {
        dstData[i][j] = val;
        continue;
      }

      int num_valid = 0;
      for (int m = i - myHalfSizeX; m <= i + myHalfSizeX; ++m) {
        int memX = m;
        if (!BndX::resolve(memX, width)) { continue; }
        for (int n = j - myHalfSizeY; n <= j + myHalfSizeY; ++n) {
          int memY = n;
          if (!BndY::resolve(memY, height)) { continue; }
          if (Constants::isGood(srcData[memX][memY])) {
            ++num_valid;
          }
        }
      }

      if (num_valid > myMinFillCount) {
        dstData[i][j] = val;
      } else {
        dstData[i][j] = Constants::MissingData;
      }
    }
  }
} // DespeckleFilter::applyFilter

void
DespeckleFilter::process(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                        dst)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst);
}
