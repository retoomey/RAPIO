#include <rCressman.h>
#include <rFactory.h>
#include <rError.h>
#include <rStrings.h>
#include <rArrayBoundary.h>
#include <vector>

using namespace rapio;

void
Cressman::introduceSelf()
{
  Factory<ArraySampler>::introduce("cressman", std::make_shared<Cressman>());
}

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

template <typename BndX, typename BndY>
void
Cressman::applyFilter(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                    & dst,
  const ArrayMapper                                    & mapper) const
{
  auto& srcData = src->ref();
  auto& dstData = dst->ref();

  int srcW    = static_cast<int>(src->getX());
  int srcH    = static_cast<int>(src->getY());
  size_t dstW = dst->getX();
  size_t dstH = dst->getY();

  // Cache invariant neighborhood parameters
  const float radX     = static_cast<float>(myWidth) / 2.0f;
  const float radY     = static_cast<float>(myHeight) / 2.0f;
  const float invRadX2 = 1.0f / (radX * radX);
  const float invRadY2 = 1.0f / (radY * radY);

  // Batch map the coordinates to eliminate virtual overhead in the inner loop
  std::vector<float> uCoords(dstW);
  std::vector<float> vCoords(dstH);

  mapper.map2D_Y(0, dstW, uCoords.data()); // map I (dstX) to U (srcX)
  mapper.map2D_X(0, dstH, vCoords.data()); // map J (dstY) to V (srcY)

  for (size_t i = 0; i < dstW; ++i) {
    float u    = uCoords[i];
    int startX = static_cast<int>(std::floor(u - radX));
    int endX   = static_cast<int>(std::ceil(u + radX));

    for (size_t j = 0; j < dstH; ++j) {
      float v    = vCoords[j];
      int startY = static_cast<int>(std::floor(v - radY));
      int endY   = static_cast<int>(std::ceil(v + radY));

      float sumWt       = 0.0f;
      float sumVal      = 0.0f;
      float currentMask = Constants::DataUnavailable;
      bool exactMatch   = false;

      for (int m = startX; m <= endX; ++m) {
        int memX = m;
        if (!BndX::resolve(memX, srcW)) { continue; }

        float dx         = static_cast<float>(m) - u;
        float dx2_scaled = (dx * dx) * invRadX2;

        if (dx2_scaled >= 1.0f) { continue; }

        for (int n = startY; n <= endY; ++n) {
          int memY = n;
          if (!BndY::resolve(memY, srcH)) { continue; }

          float dy = static_cast<float>(n) - v;
          float d2 = dx2_scaled + (dy * dy) * invRadY2;

          if (d2 <= 1.0f) {
            float val = srcData[memX][memY];

            if (Constants::isGood(val)) {
              if (d2 < std::numeric_limits<float>::epsilon()) {
                dstData[i][j] = val;
                exactMatch    = true;
                break;
              }

              float wt = (1.0f - d2) / (1.0f + d2);
              sumWt  += wt;
              sumVal += val * wt;
            } else {
              if (val == Constants::MissingData) {
                currentMask = Constants::MissingData;
              }
            }
          }
        } // End lon row

        if (exactMatch) { break; }
      } // End lat column

      if (!exactMatch) {
        if (sumWt > 0.0f) {
          dstData[i][j] = sumVal / sumWt;
        } else {
          dstData[i][j] = currentMask;
        }
      }
    } // dstY Loop
  }   // dstX Loop
} // Cressman::applyFilter

void
Cressman::remap2D(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                & dst,
  const ArrayMapper                                & mapper)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst, mapper);
}
