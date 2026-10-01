#include <rNearestNeighbor.h>
#include <rFactory.h>
#include <rArrayBoundary.h>
#include <vector>

using namespace rapio;

void
NearestNeighbor::introduceSelf()
{
  Factory<ArraySampler>::introduce("nearest", std::make_shared<NearestNeighbor>());
}

std::string
NearestNeighbor::getHelpString()
{
  return " -- Nearest Neighbor.";
}

template <typename BndX, typename BndY>
void
NearestNeighbor::applyFilter(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                           & dst,
  const ArrayMapper                                           & mapper) const
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
    float u      = uCoords[i];
    int mapped_i = static_cast<int>(u + 0.5f);

    for (size_t j = 0; j < dstH; ++j) {
      float v      = vCoords[j];
      int mapped_j = static_cast<int>(v + 0.5f);

      int memX = mapped_i;
      int memY = mapped_j;

      // Resolve boundaries
      if (!BndX::resolve(memX, srcW) || !BndY::resolve(memY, srcH)) {
        dstData[i][j] = Constants::DataUnavailable;
      } else {
        dstData[i][j] = srcData[memX][memY];
      }
    }
  }
} // NearestNeighbor::applyFilter

void
NearestNeighbor::remap2D(std::shared_ptr<Array<float, 2> >& src,
  std::shared_ptr<Array<float, 2> >                       & dst,
  const ArrayMapper                                       & mapper)
{
  if (!src || !dst) { return; }
  RAPIO_DISPATCH_BOUNDARIES(myXBoundary, myYBoundary, applyFilter, src, dst, mapper);
}
