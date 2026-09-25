#include <rDilateFilter.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;

void
DilateFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOne = std::make_shared<DilateFilter>();
  Factory<ArrayFilter>::introduce("dilate", newOne);
}

std::string
DilateFilter::getHelpString()
{
  return
    "{sizeX=3}:{sizeY=3}:{minFill=0.33}:{dilateLarge=1} -- Dilates the image based on neighborhood values. Note: Even sizes are rounded up to the nearest odd-sized kernel (e.g., 20 becomes a 21x21 window).";
}

bool
DilateFilter::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }

  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);
  try {
    if (parts.size() > 0) { mySizeX = std::stoi(parts[0]); }
    if (parts.size() > 1) { mySizeY = std::stoi(parts[1]); }
    if (parts.size() > 2) { myMinFillFrac = std::stof(parts[2]); }
    if (parts.size() > 3) { myDilateLarge = (std::stoi(parts[3]) != 0); }

    myMinFillCount = static_cast<int>(myMinFillFrac * mySizeX * mySizeY + 0.5f);
    myHalfSizeX    = mySizeX / 2;
    myHalfSizeY    = mySizeY / 2;
  } catch (const std::exception& e) {
    fLogSevere("DilateFilter param error: {}", e.what());
    return false;
  }
  return true;
}

template <typename BndX, typename BndY>
void
DilateFilter::applyFilter(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                         dst)
{
  auto& srcData = src->ref();
  auto& dstData = dst->ref();
  int width     = src->getX();
  int height    = src->getY();

  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      int num_valid     = 0;
      float best        = srcData[i][j];
      float second_best = best;

      for (int m = i - myHalfSizeX; m <= i + myHalfSizeX; ++m) {
        int memX = m;
        if (!BndX::resolve(memX, width)) { continue; }
        for (int n = j - myHalfSizeY; n <= j + myHalfSizeY; ++n) {
          int memY = n;
          if (!BndY::resolve(memY, height)) { continue; }

          float thisval = srcData[memX][memY];
          if (Constants::isGood(thisval)) {
            if (myDilateLarge) {
              if (thisval > second_best) {
                if (thisval > best) {
                  second_best = best;
                  best        = thisval;
                } else {
                  second_best = thisval;
                }
              } else if (!Constants::isGood(second_best)) {
                best        = thisval;
                second_best = best;
              }
            } else {
              if (thisval < second_best) {
                if (thisval < best) {
                  second_best = best;
                  best        = thisval;
                } else {
                  second_best = thisval;
                }
              } else if (!Constants::isGood(second_best)) {
                best        = thisval;
                second_best = best;
              }
            }
            num_valid++;
          }
        }
      }

      if (num_valid > myMinFillCount) {
        dstData[i][j] = second_best;
      } else if (Constants::isGood(srcData[i][j])) {
        dstData[i][j] = Constants::MissingData;
      } else {
        dstData[i][j] = srcData[i][j];
      }
    }
  }
} // DilateFilter::applyFilter

void
DilateFilter::process(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                     dst)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst);
}
