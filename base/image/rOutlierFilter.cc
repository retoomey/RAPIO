#include <rOutlierFilter.h>
#include <rFactory.h>
#include <rError.h>
#include <rStrings.h>
#include <numeric>
#include <cmath>

using namespace rapio;

void
OutlierFilter::introduceSelf()
{
  Factory<ArrayFilter>::introduce("outlier", std::make_shared<OutlierFilter>());
}

std::string
OutlierFilter::getHelpString()
{
  return fmt::format("{{z_thresh={:g}}}:{{fallback={:g}}} -- Replaces values exceeding Z-score with previous value.",
           myZThreshold, myFallback);
}

bool
OutlierFilter::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }
  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);
  try {
    if (parts.size() > 0) { myZThreshold = std::stof(parts[0]); }
    if (parts.size() > 1) { myFallback = std::stof(parts[1]); }
  } catch (const std::exception& e) {
    fLogSevere("OutlierFilter param error: {}", e.what());
    return false;
  }
  return true;
}

void
OutlierFilter::process1D(const std::shared_ptr<Array<float, 1> >& src,
  const std::shared_ptr<Array<float, 1> >                       & dst)
{
  if (!src || !dst) { return; }
  process1DRaw(static_cast<const float *>(src->getRawDataPointer()),
    static_cast<float *>(dst->getRawDataPointer()),
    src->refAs1D().size());
}

void
OutlierFilter::process1D(const std::vector<float>& src,
  std::vector<float>                             & dst)
{
  if (dst.size() < src.size()) {
    dst.resize(src.size());
  }
  process1DRaw(src.data(), dst.data(), src.size());
}

void
OutlierFilter::process1DRaw(const float * srcData, float * dstData, size_t size)
{
  std::copy(srcData, srcData + size, dstData);
  if (size < 3) { return; }

  double sum = 0.0, sq_sum = 0.0;
  size_t valid_count = 0;

  // Calculate mean, ignoring missing data
  for (size_t i = 0; i < size; ++i) {
    if (Constants::isGood(srcData[i])) {
      sum += srcData[i];
      valid_count++;
    }
  }

  if (valid_count < 3) { return; }
  float mean = sum / valid_count;

  // Calculate standard deviation, ignoring missing data
  for (size_t i = 0; i < size; ++i) {
    if (Constants::isGood(srcData[i])) {
      float diff = srcData[i] - mean;
      sq_sum += diff * diff;
    }
  }
  float std_dev = std::sqrt(std::max(0.0, sq_sum / valid_count));

  // Apply filter
  for (size_t i = 0; i < size; ++i) {
    if (!Constants::isGood(dstData[i])) { continue; }

    float z = std::abs(dstData[i] - mean) / (std_dev + 1e-6f);
    if (z > myZThreshold) {
      dstData[i] = (i == 0) ? myFallback : dstData[i - 1];
    }
  }
} // OutlierFilter::process1DRaw
