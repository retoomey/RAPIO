#include <rOpenCVFilter.h>
#include <rFactory.h>
#include <rError.h>

#include <algorithm>
#include <string>
#include <vector>

#ifdef HAVE_OPENCV
# include <opencv2/core.hpp>
# include <opencv2/imgproc.hpp>
#endif

using namespace rapio;

void
OpenCVFilter::introduceSelf()
{
  #ifdef HAVE_OPENCV
  std::shared_ptr<ArrayFilter> newOne = std::make_shared<OpenCVFilter>();
  Factory<ArrayFilter>::introduce("opencv", newOne);
  #endif
}

std::string
OpenCVFilter::getHelpString()
{
  return "OpenCV spatial/edge filter. Syntax 'opencv:{op}:{p1}:{p2}:{p3}:{p4}'.\n"
         "    {op=box}:{sizeX=3}:{sizeY=3}                  -- Box blur.\n"
         "    {op=gaussian}:{sizeX=3}:{sizeY=3}:{sigma=0}   -- Gaussian blur.\n"
         "    {op=median}:{ksize=3}                         -- Median blur (float <= 5).\n"
         "    {op=erode|dilate|open|close|gradient|tophat|blackhat}:{sizeX=3}:{sizeY=3}:{iter=1} -- Morphology.\n"
         "    {op=sobel}:{dx=1}:{dy=0}:{ksize=3}            -- Sobel derivative.\n"
         "    {op=scharr}:{dx=1}:{dy=0}                     -- Scharr derivative.\n"
         "    {op=laplacian}:{ksize=1}                      -- Laplacian.\n"
         "    {op=canny}:{thresh1=100}:{thresh2=200}:{aperture=3}:{l2=0} -- Canny edges (0/1 output).";
}

bool
OpenCVFilter::parseInt(const std::string& value, int& out)
{
  try {
    out = std::stoi(value);
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

bool
OpenCVFilter::parseDouble(const std::string& value, double& out)
{
  try {
    out = std::stof(value);
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

bool
OpenCVFilter::parseOptions(const std::string& params)
{
  if (params.empty()) { return true; }

  std::vector<std::string> parts;

  Strings::splitWithoutEnds(params, ':', &parts);
  if (parts.empty()) { return true; }

  const std::string op = Strings::makeLower(parts[0]);

  if ((op == "box") || (op == "blur")) {
    myOperation = Operation::Box;
  } else if ((op == "gaussian") || (op == "gauss")) {
    myOperation = Operation::Gaussian;
  } else if ((op == "median") || (op == "med")) { myOperation = Operation::Median; } else if (op == "erode") {
    myOperation = Operation::Erode;
  } else if (op == "dilate") { myOperation = Operation::Dilate; } else if (op == "open") {
    myOperation = Operation::Open;
  } else if (op == "close") { myOperation = Operation::Close; } else if ((op == "gradient") || (op == "grad")) {
    myOperation = Operation::Gradient;
  } else if (op == "tophat") { myOperation = Operation::TopHat; } else if (op == "blackhat") {
    myOperation = Operation::BlackHat;
  } else if (op == "sobel") { myOperation = Operation::Sobel; } else if (op == "scharr") {
    myOperation = Operation::Scharr;
  } else if ((op == "laplacian") || (op == "laplace")) { myOperation = Operation::Laplacian; } else if (op == "canny") {
    myOperation = Operation::Canny;
  } else {
    fLogSevere("OpenCVFilter unknown operation '{}'.", parts[0]);
    return false;
  }

  // Optional argument readers.  Missing arguments leave the current default.
  auto getInt = [&](size_t index, int& out) -> bool {
      if (parts.size() <= index) { return true; }
      if (!parseInt(parts[index], out)) {
        fLogSevere("OpenCVFilter bad integer '{}'.", parts[index]);
        return false;
      }
      return true;
    };
  auto getDouble = [&](size_t index, double& out) -> bool {
      if (parts.size() <= index) { return true; }
      if (!parseDouble(parts[index], out)) {
        fLogSevere("OpenCVFilter bad number '{}'.", parts[index]);
        return false;
      }
      return true;
    };

  try {
    switch (myOperation) {
        case Operation::Box:
        case Operation::Gaussian:
        case Operation::Erode:
        case Operation::Dilate:
        case Operation::Open:
        case Operation::Close:
        case Operation::Gradient:
        case Operation::TopHat:
        case Operation::BlackHat:
          if (!getInt(1, mySizeX)) { return false; }
          if (!getInt(2, mySizeY)) { return false; }
          if (!getDouble(3, myParam)) { return false; }
          if (mySizeX < 1) { mySizeX = 1; }
          if (mySizeY < 1) { mySizeY = 1; }
          if ((mySizeX % 2) == 0) { ++mySizeX; }
          if ((mySizeY % 2) == 0) { ++mySizeY; }
          break;

        case Operation::Median:
          if (!getInt(1, mySizeX)) { return false; }
          if (mySizeX < 3) { mySizeX = 3; }
          if ((mySizeX % 2) == 0) { ++mySizeX; }
          mySizeY = mySizeX;
          break;

        case Operation::Sobel:
          myKSize = 3;
          if (!getInt(1, myDx)) { return false; }
          if (!getInt(2, myDy)) { return false; }
          if (!getInt(3, myKSize)) { return false; }
          if (myKSize < 1) { myKSize = 1; }
          if ((myKSize % 2) == 0) { ++myKSize; }
          break;

        case Operation::Scharr:
          myKSize = 3;
          if (!getInt(1, myDx)) { return false; }
          if (!getInt(2, myDy)) { return false; }
          break;

        case Operation::Laplacian:
          myKSize = 1;
          if (!getInt(1, myKSize)) { return false; }
          if (myKSize < 1) { myKSize = 1; }
          if ((myKSize % 2) == 0) { ++myKSize; }
          break;

        case Operation::Canny:
          myKSize = 3;
          if (!getDouble(1, myThreshold1)) { return false; }
          if (!getDouble(2, myThreshold2)) { return false; }
          if (!getInt(3, myKSize)) { return false; }
          if (parts.size() > 4) {
            int l2 = 0;

            if (!parseInt(parts[4], l2)) { fLogSevere("OpenCVFilter bad l2 flag '{}'.", parts[4]); return false; }
            myL2Gradient = (l2 != 0);
          }
          if ((myKSize != 3) && (myKSize != 5) && (myKSize != 7)) {
            fLogSevere("OpenCVFilter canny aperture must be 3, 5 or 7; using 3.");
            myKSize = 3;
          }
          break;
    }

    if ((myDx < 0) || (myDy < 0) || (myDx > 2) || (myDy > 2)) {
      fLogSevere("OpenCVFilter derivative order must be 0..2.");
      return false;
    }
  } catch (const std::exception& e) {
    fLogSevere("OpenCVFilter param error: {}", e.what());
    return false;
  }
  return true;
} // OpenCVFilter::parseOptions

#ifdef HAVE_OPENCV
namespace {
/** Fill a floating point data matrix with missing values replaced by zero,
 * and build a parallel 0/1 validity mask. */
void
fillDataAndMask(const cv::Mat& src, cv::Mat& data, cv::Mat& mask)
{
  const int rows = src.rows;
  const int cols = src.cols;

  data.create(rows, cols, CV_32F);
  mask.create(rows, cols, CV_32F);

  for (int i = 0; i < rows; ++i) {
    const float * s = src.ptr<float>(i);
    float * d       = data.ptr<float>(i);
    float * m       = mask.ptr<float>(i);

    for (int j = 0; j < cols; ++j) {
      if (Constants::isGood(s[j])) {
        d[j] = s[j];
        m[j] = 1.0f;
      } else {
        d[j] = 0.0f;
        m[j] = 0.0f;
      }
    }
  }
}

/** Copy the filtered result out, restoring missing sentinels where the
 * input mask was invalid. */
void
copyResult(const cv::Mat& result, const cv::Mat& mask, cv::Mat& dst)
{
  const int rows = result.rows;
  const int cols = result.cols;

  for (int i = 0; i < rows; ++i) {
    const float * r = result.ptr<float>(i);
    const float * m = mask.ptr<float>(i);
    float * o       = dst.ptr<float>(i);

    for (int j = 0; j < cols; ++j) {
      o[j] = (m[j] > 0.0f) ? r[j] : static_cast<float>(Constants::MissingData);
    }
  }
}

/** Normalize a linearly filtered data matrix by the identically filtered
 * mask, giving a mean over valid neighbors only. */
void
normalizeByMask(cv::Mat& result, const cv::Mat& weight)
{
  const int rows = result.rows;
  const int cols = result.cols;

  for (int i = 0; i < rows; ++i) {
    float * r       = result.ptr<float>(i);
    const float * w = weight.ptr<float>(i);

    for (int j = 0; j < cols; ++j) {
      r[j] = (w[j] > 1e-6f) ? (r[j] / w[j]) : 0.0f;
    }
  }
}
}

void
OpenCVFilter::process2D(const std::shared_ptr<Array<float, 2> >& src,
  const std::shared_ptr<Array<float, 2> >                      & dst)
{
  if (!src || !dst) { return; }

  const int rows = static_cast<int>(src->getX());
  const int cols = static_cast<int>(src->getY());

  if ((rows <= 0) || (cols <= 0)) { return; }

  cv::Mat srcMat(rows, cols, CV_32F, src->getRawDataPointer());
  cv::Mat dstMat(rows, cols, CV_32F, dst->getRawDataPointer());

  cv::Mat data, mask;
  bool cacheHit = false;

  // 1. Smart Cache Check
  if (myPipelineState) {
    auto validPtr = myPipelineState->get<void *>("opencv_valid_ptr");
    if (validPtr && (*validPtr == src->getRawDataPointer())) {
      auto optData = myPipelineState->get<cv::Mat>("opencv_data");
      auto optMask = myPipelineState->get<cv::Mat>("opencv_mask");
      if (optData && optMask) {
        data     = *optData;
        mask     = *optMask;
        cacheHit = true;
      }
    }
  }

  // 2. Extract only if cache missed
  if (!cacheHit) {
    fillDataAndMask(srcMat, data, mask);
  }

  cv::Mat result(rows, cols, CV_32F);
  const cv::Size ksize(mySizeY, mySizeX);
  const cv::Point anchor(-1, -1);

  switch (myOperation) {
      case Operation::Box: {
        cv::Mat weight;

        cv::blur(data, result, ksize, anchor, cv::BORDER_REPLICATE);
        cv::blur(mask, weight, ksize, anchor, cv::BORDER_REPLICATE);
        normalizeByMask(result, weight);
        break;
      }
      case Operation::Gaussian: {
        cv::Mat weight;
        const double sigma = (myParam > 0.0) ? myParam : 0.0;

        cv::GaussianBlur(data, result, ksize, sigma, sigma, cv::BORDER_REPLICATE);
        cv::GaussianBlur(mask, weight, ksize, sigma, sigma, cv::BORDER_REPLICATE);
        normalizeByMask(result, weight);
        break;
      }
      case Operation::Median: {
        int k = std::max(mySizeX, mySizeY);

        if ((k % 2) == 0) { ++k; }
        if (k > 5) {
          fLogSevere("OpenCVFilter median float kernels support up to 5; clamping {} to 5.", k);
          k = 5;
        }
        cv::medianBlur(data, result, k);
        break;
      }
      case Operation::Sobel: {
        cv::Sobel(data, result, CV_32F, myDx, myDy, myKSize, 1, 0, cv::BORDER_REPLICATE);
        break;
      }
      case Operation::Scharr: {
        cv::Scharr(data, result, CV_32F, myDx, myDy, 1, 0, cv::BORDER_REPLICATE);
        break;
      }
      case Operation::Laplacian: {
        cv::Laplacian(data, result, CV_32F, myKSize, 1, 0, cv::BORDER_REPLICATE);
        break;
      }
      case Operation::Canny: {
        cv::Mat data8U, edges8U;
        cv::normalize(data, data8U, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::Canny(data8U, edges8U, myThreshold1, myThreshold2, myKSize, myL2Gradient);
        edges8U.convertTo(result, CV_32F, 1.0 / 255.0);
        break;
      }
      default: {
        int type = cv::MORPH_ERODE;
        switch (myOperation) {
            case Operation::Erode: type = cv::MORPH_ERODE;
              break;
            case Operation::Dilate: type = cv::MORPH_DILATE;
              break;
            case Operation::Open: type = cv::MORPH_OPEN;
              break;
            case Operation::Close: type = cv::MORPH_CLOSE;
              break;
            case Operation::Gradient: type = cv::MORPH_GRADIENT;
              break;
            case Operation::TopHat: type = cv::MORPH_TOPHAT;
              break;
            case Operation::BlackHat: type = cv::MORPH_BLACKHAT;
              break;
            default: break;
        }

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, ksize);
        int iterations = (myParam > 0.0) ? static_cast<int>(myParam) : 1;

        if (iterations < 1) { iterations = 1; }
        cv::morphologyEx(data, result, type, kernel, anchor, iterations, cv::BORDER_REPLICATE);
        break;
      }
  }

  copyResult(result, mask, dstMat);

  // 3. Update the cache for the next filter in the pipeline
  if (myPipelineState) {
    myPipelineState->put<void *>("opencv_valid_ptr", dst->getRawDataPointer());
    myPipelineState->put<cv::Mat>("opencv_data", result);
    myPipelineState->put<cv::Mat>("opencv_mask", mask);
  }
} // OpenCVFilter::process2D

#else // ifdef HAVE_OPENCV
void
OpenCVFilter::process2D(const std::shared_ptr<Array<float, 2> >& src,
  const std::shared_ptr<Array<float, 2> >                      & dst)
{
  fLogSevere("OpenCVFilter used but RAPIO was built without OpenCV support.");
}

#endif // ifdef HAVE_OPENCV
