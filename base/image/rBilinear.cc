#include <rBilinear.h>
#include <rError.h>
#include <rFactory.h>
#include <rStrings.h>
#include <rArrayBoundary.h>
#include <vector>

using namespace rapio;

void
Bilinear::introduceSelf()
{
  Factory<ArraySampler>::introduce("bilinear", std::make_shared<Bilinear>());
}

bool
Bilinear::parseOptions(const std::string& params)
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
      fLogSevere("Warning: 'bilinear' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 2);
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

template <typename BndX, typename BndY>
void
Bilinear::applyFilter(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                    & dst,
  const ArrayMapper                                    & mapper) const
{
  auto& srcData = src->ref();
  auto& dstData = dst->ref();

  int srcW    = static_cast<int>(src->getX());
  int srcH    = static_cast<int>(src->getY());
  size_t dstW = dst->getX();
  size_t dstH = dst->getY();

  // Batch map the coordinates to eliminate virtual overhead in the inner loop
  std::vector<float> uCoords(dstW);
  std::vector<float> vCoords(dstH);

  mapper.map2D_Y(0, dstW, uCoords.data()); // map I (dstX) to U (srcX)
  mapper.map2D_X(0, dstH, vCoords.data()); // map J (dstY) to V (srcY)

  for (size_t i = 0; i < dstW; ++i) {
    float u = uCoords[i];

    for (size_t j = 0; j < dstH; ++j) {
      float v = vCoords[j];

      int iu = static_cast<int>(std::floor(u));
      int iv = static_cast<int>(std::floor(v));

      int i00_x = iu, i00_y = iv;
      int i10_x = iu + 1, i10_y = iv;
      int i01_x = iu, i01_y = iv + 1;
      int i11_x = iu + 1, i11_y = iv + 1;

      // Resolve boundaries for the 4 corners
      if (!BndX::resolve(i00_x, srcW) || !BndY::resolve(i00_y, srcH) ||
        !BndX::resolve(i10_x, srcW) || !BndY::resolve(i10_y, srcH) ||
        !BndX::resolve(i01_x, srcW) || !BndY::resolve(i01_y, srcH) ||
        !BndX::resolve(i11_x, srcW) || !BndY::resolve(i11_y, srcH))
      {
        continue; // Out of bounds, leave destination initialized to missing
      }

      float p00 = srcData[i00_x][i00_y];
      float p10 = srcData[i10_x][i10_y];
      float p01 = srcData[i01_x][i01_y];
      float p11 = srcData[i11_x][i11_y];

      // Fallback to nearest neighbor if any corner is missing/bad data
      if (!Constants::isGood(p00) || !Constants::isGood(p10) ||
        !Constants::isGood(p01) || !Constants::isGood(p11))
      {
        int nearest_i = static_cast<int>(u + 0.5f);
        int nearest_j = static_cast<int>(v + 0.5f);

        if (BndX::resolve(nearest_i, srcW) && BndY::resolve(nearest_j, srcH)) {
          dstData[i][j] = srcData[nearest_i][nearest_j];
        } else {
          dstData[i][j] = Constants::DataUnavailable;
        }
        continue;
      }

      float fx = u - static_cast<float>(iu);
      float fy = v - static_cast<float>(iv);
      float nx = 1.0f - fx;
      float ny = 1.0f - fy;

      // Standard bilinear interpolation math
      dstData[i][j] = p00 * (nx * ny) + p10 * (fx * ny) + p01 * (nx * fy) + p11 * (fx * fy);
    }
  }
} // Bilinear::applyFilter

void
Bilinear::remap2D(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                & dst,
  const ArrayMapper                                & mapper)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst, mapper);
}
