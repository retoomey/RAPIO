#pragma once
#include <rArrayFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>

namespace rapio {
/**
 * @class OpenCVFilter
 * @brief A generic 2D spatial filter backed by OpenCV.
 *
 * @details This filter bridges RAPIO float arrays into OpenCV matrices and
 * applies one of several common spatial operations (box/gaussian blur,
 * median blur, morphology or the edge detectors).  It is registered with
 * the ArrayFilter factory under the name 'opencv' so it can be chained
 * inside an ArrayPipeline, for example:
 *
 *   auto p = ArrayPipeline::create("opencv:gaussian:5:5:1.5");
 *   auto p2 = ArrayPipeline::create("threshold:30:100,opencv:median:3");
 *   auto p3 = ArrayPipeline::create("opencv:sobel:1:0:3");
 *   auto p4 = ArrayPipeline::create("opencv:canny:50:150:3");
 *
 * Missing data sentinels (Constants::MissingData, NaN, etc.) are masked out
 * before handing the data to OpenCV and restored in the output, so they do
 * not bleed into neighboring valid pixels.
 *
 * @ingroup rapio_image
 */
class OpenCVFilter : public ArrayFilter {
public:
  OpenCVFilter() = default;
  virtual
  ~OpenCVFilter() = default;

  /** Spawns a new independent instance for pipeline execution. */
  virtual std::shared_ptr<ArrayFilter>
  create() const override
  {
    return std::make_shared<OpenCVFilter>();
  }

  /** Registers this filter with the ArrayFilter factory. */
  static void
  introduceSelf();

  std::string
  getHelpString() override;

  bool
  parseOptions(const std::string& params) override;

  /** OpenCV operations here are 2D spatial filters. */
  bool
  supportsDimensions(size_t dims) const override
  {
    return (dims == 2);
  }

  void
  process2D(const std::shared_ptr<Array<float, 2> >& src,
    const std::shared_ptr<Array<float, 2> >        & dst) override;

private:

  /** The set of supported OpenCV operations. */
  enum class Operation {
    Box,
    Gaussian,
    Median,
    Erode,
    Dilate,
    Open,
    Close,
    Gradient,
    TopHat,
    BlackHat,
    Sobel,
    Scharr,
    Laplacian,
    Canny
  };

  /** Parse a single parameter as an integer with a fallback default. */
  static bool
  parseInt(const std::string& value, int& out);

  /** Parse a single parameter as a double with a fallback default. */
  static bool
  parseDouble(const std::string& value, double& out);

  /** Operation selected from the config string. */
  Operation myOperation = Operation::Gaussian;

  /** Kernel size in X (first array dimension / matrix rows). */
  int mySizeX = 3;

  /** Kernel size in Y (second array dimension / matrix columns). */
  int mySizeY = 3;

  /** Generic operation parameter: gaussian sigma, or morphology iterations. */
  double myParam = 0.0;

  /** X derivative order for sobel/scharr. */
  int myDx = 1;

  /** Y derivative order for sobel/scharr. */
  int myDy = 0;

  /** Aperture size for sobel/laplacian/canny (must be odd). */
  int myKSize = 3;

  /** First hysteresis threshold for canny. */
  double myThreshold1 = 100.0;

  /** Second hysteresis threshold for canny. */
  double myThreshold2 = 200.0;

  /** Use the more accurate L2 gradient in canny. */
  bool myL2Gradient = false;
};
} // namespace rapio
