#include "rNIDSUtil.h"
#include "rConstants.h"
#include "rError.h"

#include <algorithm>
#include <cassert>

using namespace rapio;
using namespace std;

static const int SECONDS_PER_DAY = 86400;

Time
NIDSUtil::getTimeFromNIDSTime(short volScanDate, int volScanStartTime)
{
  // long totalSeconds = ((long)(volScanDate - 1) * SECONDS_PER_DAY) + volScanStartTime;
  //    long totalSeconds = ((long)(volScanDate - 1) * SECONDS_PER_DAY) + volScanStartTime;
  return Time::SecondsSinceEpoch(volScanStartTime + ((volScanDate - 1) * 24 * 3600));
}

void
NIDSUtil::getNIDSTimeFromTime(const Time& t, short& volScanDate, int& volScanStartTime)
{
  long totalSeconds = t.getSecondsSinceEpoch();

  // Reverse the (volScanDate - 1) logic
  volScanDate = (short) ((totalSeconds / SECONDS_PER_DAY) + 1);

  // The remainder is the start time within that day
  volScanStartTime = (int) (totalSeconds % SECONDS_PER_DAY);
}

void
NIDSUtil::getRLEColors(const std::vector<char> & src, std::vector<int> & data)
{
  for (int i = 0; i < (signed) src.size(); i++) {
    unsigned int run;
    short int color;

    // decode the first byte
    run = src[i] >> 4; // shift left to get higher 4 bits
    run = run & 0x0F;
    // make sure that shifting does not introduce 1's at high bits.

    color = src[i] & 0x0F; // get lower 4 bits
    #if 0
    if ( (i > 0) && (i < 10) ) {
      std::cout << "BITS " << src[i] << ":";
      std::cout << BitsPrint<char>(src [i]) << endl;
      std::cout << "run " << run << " color " << color << endl;
    }

    #endif
    // higher 4 bits are the repeat numbers
    // lower 4 bits are the color codes
    for (int j = 0; j < (signed) run; j++) {
      data.push_back(color);
    }
  }
}

void
NIDSUtil::getColors(const std::vector<char> & src,
  std::vector<int>                          & data)
{
  int N = src.size();

  for (int i = 0; i < (N - 1); i += 2) {
    // MSB of the short first, then the LSB
    data.push_back( (unsigned char) src[i]);
    data.push_back( (unsigned char) src[i + 1]);
  }
}

void
NIDSUtil::getGenericColors(const std::vector<char> & src,
  std::vector<int>                                 & data)
{
  typedef union short_or_char {
    short s;
    char  c[2];
  } ShortOrChar_t;

  int N = src.size();
  ShortOrChar_t tmp;

  for (int i = 0; i < N; i += 2) {
    tmp.c[0] = src[i];
    tmp.c[1] = src[i + 1];
    data.push_back( (int) tmp.s);
  }
}

void
NIDSUtil::colorToValueD1(
  const std::vector<int>   & colors,
  const std::vector<float> & thresholds,
  std::vector<float>       & values)
{
  // Originally for Product 164
  // Took 165 and 177 out since scaling by 10 seems to mess it up as per Steve Honey @ WSI

  /* These are simple code levels that should be scaled
   * instead of being thresholded */
  for (size_t i = 0; i < colors.size(); ++i) {
    values.push_back(colors[i] / 10.0);
  }
}

void
NIDSUtil::colorToValueD2(
  const std::vector<int>   & colors,
  const std::vector<float> & thresholds,
  std::vector<float>       & values)
{
  // Originally for Product 165, 176, 177

  /* These are simple code levels that should be passed
   * through instead of being thresholded */
  for (size_t i = 0; i < colors.size(); ++i) {
    values.push_back(colors[i]);
  }
}

void
NIDSUtil::colorToValueD3(
  const std::vector<int>   & colors,
  const std::vector<float> & thresholds,
  std::vector<float>       & values)
{
  // Originally for Product
  // 94, 99, 153, 154, 155, 159, 161, 163

  /* this is for FSI compatability with AWIPS,
   * which doesn't use averaging for this product */
  for (size_t i = 0; i < colors.size(); ++i) {
    values.push_back(thresholds[ colors[i] ]);
  }
}

void
NIDSUtil::valueToColorD3(
  const std::vector<float>& thresholds,
  const float *           values,
  const size_t            size,
  std::vector<int>        & colors)
{
  // Originally for Product
  // 94, 99, 153, 154, 155, 159, 161, 163

  const bool m = (values[0] == Constants::MissingData);
  const bool r = (values[1] == Constants::RangeFolded);

  /* this is for FSI compatability with AWIPS,
   * which doesn't use averaging for this product */
  for (size_t i = 0; i < size; ++i) {
    int color;
    // Handle specials
    if (m && (values[i] == Constants::MissingData)) {
      color = 0;
    } else if (values[i] == Constants::RangeFolded) {
      color = r ? 1 : 0;
    } else {
      auto it = std::upper_bound(thresholds.begin(), thresholds.end(), values[i]);
      color = (it - thresholds.begin()) - 1;
    }
    colors.push_back(color);
  }
}

void
NIDSUtil::colorToValueD4(
  const std::vector<int>   & colors,
  const std::vector<float> & thresholds,
  std::vector<float>       & values)
{
  // Fallback for products
  for (size_t i = 0; i < colors.size(); ++i) {
    double val;
    if ( (!thresholds.empty()) &&
      (thresholds [ colors[i] ] <= -99900 ) )
    {
      val = thresholds[ colors[i] ];
    } else {
      const float left = thresholds[ colors[i] ];
      if ((i + 1) >= (int) thresholds.size()) {
        val = left;
      } else {
        const float right = thresholds[ colors[i] + 1 ];
        val = ( left + right ) / 2.0;
      }
    }
    values.push_back(val);
  }
}

void
NIDSUtil::colorToValueE1(const std::vector<int> & color,
  std::array<short, 16>                         & data_level_threshold,
  std::vector<float>                            & values
)
{
  // Originally for Product 134
  int size = color.size();

  /** This block of code are modified from code given by Eddie
   * Forren */
  double dvl_sign, dvl_exp, dvl_mant, value;
  float linear_coeff, linear_offset, log_coeff, log_start, log_offset;

  // get linear_coeff
  dvl_sign = (data_level_threshold[0] & 0x8000) >> 15;
  dvl_exp  = (data_level_threshold[0] & 0x7c00) >> 10;
  dvl_exp  = dvl_exp - 16;
  dvl_mant = data_level_threshold[0] & 0x03ff;
  value    = pow((double) -1.0, dvl_sign)
    * pow((double) 2.0, dvl_exp)
    * (1.0 + dvl_mant / 1024.0);
  linear_coeff = value;

  // get linear_offset
  dvl_sign = (data_level_threshold[1] & 0x8000) >> 15;
  dvl_exp  = (data_level_threshold[1] & 0x7c00) >> 10;
  dvl_exp  = dvl_exp - 16;
  dvl_mant = data_level_threshold[1] & 0x03ff;
  value    = pow((double) -1.0, dvl_sign)
    * pow((double) 2.0, dvl_exp)
    * (1.0 + dvl_mant / 1024.0);
  linear_offset = value;

  // get log_start
  log_start = data_level_threshold[2];

  // get log_coeff
  dvl_sign = (data_level_threshold[3] & 0x8000) >> 15;
  dvl_exp  = (data_level_threshold[3] & 0x7c00) >> 10;
  dvl_exp  = dvl_exp - 16;
  dvl_mant = data_level_threshold[3] & 0x03ff;
  value    = pow((double) -1.0, dvl_sign)
    * pow((double) 2.0, dvl_exp)
    * (1.0 + dvl_mant / 1024.0);
  log_coeff = value; // get log_offset
  dvl_sign  = (data_level_threshold[4] & 0x8000) >> 15;
  dvl_exp   = (data_level_threshold[4] & 0x7c00) >> 10;
  dvl_exp   = dvl_exp - 16;
  dvl_mant  = data_level_threshold[4] & 0x03ff;
  value     = pow((double) -1.0, dvl_sign)
    * pow((double) 2.0, dvl_exp)
    * (1.0 + dvl_mant / 1024.0);
  log_offset = value;

  for (int i = 0; i < size; i++) {
    if (color[i] == 255) {
      values.push_back(Constants::MissingData);
    } else if (color[i] == 254) {
      values.push_back(80.0);
    } else if (color[i] < (int) log_start) {
      values.push_back
        ( (color[i] - linear_offset) / linear_coeff);
    } else {
      values.push_back
        (exp((color[i] - log_offset) / log_coeff) );
    }
  }
} // NIDSUtil::colorToValueE1

void
NIDSUtil::colorToValueE2(const std::vector<int> & color,
  std::array<short, 16>                         & data_level_threshold,
  std::vector<float>                            & values
)
{
  // Originally for Product 135
  int size = color.size();

  const uint16_t DATA_MASK   = data_level_threshold[0];
  const uint16_t DATA_SCALE  = data_level_threshold[1];
  const uint16_t DATA_OFFSET = data_level_threshold[2];

  // const uint16_t TOPPED_MASK = data_level_threshold[3];

  for (int i = 0; i < size; i++) {
    if (color[i] == 0) {
      values.push_back(Constants::MissingData);
    } else if (color[i] == 1) {
      values.push_back(Constants::MissingData);
    } else {
      values.push_back
        (((color[i] & DATA_MASK) / DATA_SCALE) - DATA_OFFSET);
    }
  }
}

void
NIDSUtil::colorToValueE3(const std::vector<int> & color,
  std::array<short, 16>                         & data_level_threshold,
  std::vector<float>                            & values
)
{
  // Originally for Product 176
  int size = color.size();

  union { uint16_t i[2];
          float    f;
  } u;
  assert(sizeof(float) == 4);
  assert(sizeof(u) == 4);

  u.i[0] = data_level_threshold[1];
  u.i[1] = data_level_threshold[0];
  const float scale = u.f;

  u.i[0] = data_level_threshold[3];
  u.i[1] = data_level_threshold[2];
  const float offset = u.f;

  float retVal = 0.0;

  for (int i = 0; i < size; i++) {
    retVal = static_cast<float>(((color[i] - offset) / scale));
    values.push_back(retVal);
  }
}
