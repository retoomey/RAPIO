#include <rSavitzkyGolayFilter.h>
#include <rFactory.h>
#include <rError.h>
#include <algorithm>

using namespace rapio;

void
SavitzkyGolayFilter::introduceSelf()
{
  Factory<ArrayFilter>::introduce("sgolay", std::make_shared<SavitzkyGolayFilter>());
}

std::string
SavitzkyGolayFilter::getHelpString()
{
  return " -- Applies a 7-point causal Savitzky-Golay smoothing filter (1D only).";
}

bool
SavitzkyGolayFilter::parseOptions(const std::string& params)
{
  return true;
}

void
SavitzkyGolayFilter::process1D(const std::shared_ptr<Array<float, 1> >& src,
  const std::shared_ptr<Array<float, 1> >                             & dst)
{
  if (!src || !dst) { return; }
  process1DRaw(static_cast<const float *>(src->getRawDataPointer()),
    static_cast<float *>(dst->getRawDataPointer()),
    src->refAs1D().size());
}

void
SavitzkyGolayFilter::process1D(const std::vector<float>& src,
  std::vector<float>                                   & dst)
{
  if (dst.size() < src.size()) {
    dst.resize(src.size());
  }
  process1DRaw(src.data(), dst.data(), src.size());
}

void
SavitzkyGolayFilter::process1DRaw(const float * srcData, float * dstData, size_t size)
{
  std::copy(srcData, srcData + size, dstData);
  if (size < 3) { return; }

  for (size_t i = 2; i < std::min<size_t>(7, size); ++i) {
    dstData[i] = (srcData[i] + srcData[i - 1] + srcData[i - 2]) / 3.0f;
  }

  const float weights[7] = {
    31.0f / 42.0f, 9.0f / 42.0f, -3.0f / 42.0f, -5.0f / 42.0f,
    -4.0f / 42.0f, 0.0f / 42.0f, 14.0f / 42.0f
  };

  for (size_t i = 6; i < size; ++i) {
    float sg_val = 0.0f;
    for (size_t w = 0; w < 7; ++w) {
      sg_val += srcData[i - w] * weights[w];
    }
    dstData[i] = sg_val;
  }
}
