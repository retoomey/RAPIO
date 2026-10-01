#define BOOST_TEST_MODULE PipelineTestSuite
#include "rBOOSTTest.h"
#include "rArrayPipeline.h"
#include "rConstants.h"
#include "rArray.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(PIPELINE_TESTS)

BOOST_AUTO_TEST_CASE(TEST_THRESHOLD)
{
  ArrayPipeline::introduceSelf();
  auto pipeline = ArrayPipeline::create("threshold:30:100");
  BOOST_REQUIRE(pipeline);
  
  auto src = Arrays::CreateFloat2D(3, 3);
  auto& data = src->ref();
  
  data[0][0] = 15.0f;  // Below min -> should become MissingData
  data[0][1] = 50.0f;  // Inside range -> should stay 50.0
  data[0][2] = 150.0f; // Above max -> should clamp to 100.0
  
  pipeline->processInPlace(src);
  
  BOOST_CHECK_EQUAL(data[0][0], static_cast<float>(Constants::MissingData));
  BOOST_CHECK_EQUAL(data[0][1], 50.0f);
  BOOST_CHECK_EQUAL(data[0][2], 100.0f);
}

BOOST_AUTO_TEST_CASE(TEST_DESPECKLE)
{
  ArrayPipeline::introduceSelf();
  // despeckle:1:1:0.5 -> 3x3 window, requires 50% (4 pixels) to keep data
  auto pipeline = ArrayPipeline::create("despeckle:1:1:0.5");
  BOOST_REQUIRE(pipeline);
  
  auto src = Arrays::CreateFloat2D(5, 5);
  src->fill(Constants::MissingData);
  auto& data = src->ref();
  
  // Create an isolated speckle (1 valid pixel)
  data[1][1] = 40.0f; 
  
  // Create a valid block (4 valid pixels)
  data[3][3] = 40.0f; data[3][4] = 40.0f;
  data[4][3] = 40.0f; data[4][4] = 40.0f;
  
  pipeline->processInPlace(src);
  
  BOOST_CHECK_EQUAL(data[1][1], static_cast<float>(Constants::MissingData));
  BOOST_CHECK_EQUAL(data[3][3], 40.0f);
}

BOOST_AUTO_TEST_CASE(TEST_PERCENT)
{
  ArrayPipeline::introduceSelf();
  // percent:50:1 -> median filter with half-size 1 (3x3 window)
  auto pipeline = ArrayPipeline::create("percent:50:1");
  BOOST_REQUIRE(pipeline);
  
  auto src = Arrays::CreateFloat2D(3, 3);
  auto& data = src->ref();
  
  // Center is 100, surrounded by 10s. Median should be 10.
  src->fill(10.0f);
  data[1][1] = 100.0f;
  
  pipeline->processInPlace(src);
  
  BOOST_CHECK_EQUAL(data[1][1], 10.0f);
}

BOOST_AUTO_TEST_CASE(TEST_DILATE)
{
  ArrayPipeline::introduceSelf();
  // dilate:3:3:0.0:1 -> 3x3 window, 0 minimum fill, dilate large values
  auto pipeline = ArrayPipeline::create("dilate:3:3:0.0:1");
  BOOST_REQUIRE(pipeline);
  
  auto src = Arrays::CreateFloat2D(5, 5);
  src->fill(Constants::MissingData);
  auto& data = src->ref();
  
  // Legacy dilate uses second_best to prevent isolated spikes from dilating.
  // We need at least two valid pixels to trigger a dilation expansion.
  data[2][2] = 50.0f;
  data[2][3] = 40.0f;
  
  pipeline->processInPlace(src);
  
  // The missing pixel at (1,2) looks at its 3x3 window, sees 50 and 40.
  // Best = 50, Second Best = 40. Since minFill is 0, it populates with 40!
  BOOST_CHECK_EQUAL(data[1][2], 40.0f);
}

BOOST_AUTO_TEST_CASE(TEST_FULL_PIPELINE)
{
  ArrayPipeline::introduceSelf();
  std::string config = "threshold:30:100,despeckle:15:15:0.75,percent:50:1,dilate:20:20";
  auto pipeline = ArrayPipeline::create(config);
  BOOST_REQUIRE(pipeline);

  auto src = Arrays::CreateFloat2D(100, 100);
  src->fill(Constants::MissingData);
  auto& data = src->ref();

  data[10][10] = 20.0f; 
  data[20][20] = 150.0f; 
  
  for (int i = 40; i < 80; ++i) {
    for (int j = 40; j < 80; ++j) {
      data[i][j] = 80.0f;
    }
  }
  data[50][50] = 120.0f;

  pipeline->processInPlace(src);

  BOOST_CHECK_EQUAL(data[10][10], static_cast<float>(Constants::MissingData));
  BOOST_CHECK_EQUAL(data[20][20], static_cast<float>(Constants::MissingData));
  BOOST_CHECK_LE(data[50][50], 100.0f);
  
  // Verify dilation expanded the storm core outward.
  // 0.33 minFill on a 20x20 kernel will only expand a straight edge by ~3 pixels. 
  // We check 2 pixels outward (row 38).
  bool validExpansion = (data[38][50] == 80.0f || data[38][50] == 100.0f);
  BOOST_CHECK(validExpansion);
}

BOOST_AUTO_TEST_CASE(TEST_OUTLIER_1D)
{
  ArrayPipeline::introduceSelf();
  // Z-thresh = 1.5, fallback = 0.0
  auto pipeline = ArrayPipeline::create("outlier:1.5:0.0");
  BOOST_REQUIRE(pipeline);

  auto src = Arrays::CreateFloat1D(8);
  auto& data = src->ref();
  
  for(int i = 0; i < 8; ++i) {
    data[i] = 10.0f;
  }
  // Inject a massive outlier spike
  data[4] = 100.0f; 
  
  pipeline->processInPlace(src);

  // The Z-score of 100 will exceed 1.5, so the outlier filter should replace it 
  // with the previous valid value (data[3], which is 10.0f)[cite: 1]
  BOOST_CHECK_EQUAL(data[4], 10.0f);
}

BOOST_AUTO_TEST_CASE(TEST_SGOLAY_1D)
{
  ArrayPipeline::introduceSelf();
  // 7-point causal Savitzky-Golay filter
  auto pipeline = ArrayPipeline::create("sgolay");
  BOOST_REQUIRE(pipeline);

  auto src = Arrays::CreateFloat1D(10);
  auto& data = src->ref();
  
  // Linear ramp
  for(int i = 0; i < 10; ++i) {
    data[i] = static_cast<float>(i);
  }
  
  pipeline->processInPlace(src);
  
  // Test the initial causal startup window which averages the previous 3 elements
  // data[2] should be (0 + 1 + 2) / 3 = 1.0
  BOOST_CHECK_EQUAL(data[2], 1.0f);
  
  // The causal SGolay with these specific weights outputs exactly 280/42 (~6.666) 
  // for this specific linear ramp at index 8.
  BOOST_CHECK_CLOSE(data[8], 280.0f / 42.0f, 0.1f);
}

BOOST_AUTO_TEST_CASE(TEST_VECTOR_1D_PATHWAY)
{
  ArrayPipeline::introduceSelf();
  
  // Test 1: In-place vector processing
  // This specifically tests the memory-aliasing fix where &src == &dst
  auto pipelineOutlier = ArrayPipeline::create("outlier:1.5:0.0");
  BOOST_REQUIRE(pipelineOutlier);

  std::vector<float> inPlaceData = {10.0f, 10.0f, 10.0f, 10.0f, 100.0f, 10.0f, 10.0f, 10.0f};
  pipelineOutlier->process(inPlaceData, inPlaceData);
  
  // The outlier (100.0f) should be replaced by the previous value (10.0f)
  BOOST_CHECK_EQUAL(inPlaceData[4], 10.0f);

  // Test 2: Out-of-place vector processing with multiple filters
  // This tests the ping-pong temp vector routing for an even/odd number of filters
  auto pipelineMulti = ArrayPipeline::create("sgolay,outlier:2.0:0.0");
  BOOST_REQUIRE(pipelineMulti);

  std::vector<float> srcData = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 100.0f, 7.0f, 8.0f};
  std::vector<float> dstData; // Intentionally empty to test automatic resizing
  
  pipelineMulti->process(srcData, dstData);
  
  // Check that destination vector was properly resized and populated
  BOOST_REQUIRE_EQUAL(dstData.size(), srcData.size());
  
  // The SGolay startup window averages the previous 3 elements.
  // dstData[2] should be (src[2] + src[1] + src[0]) / 3 = (3+2+1)/3 = 2.0
  BOOST_CHECK_EQUAL(dstData[2], 2.0f);

  // The massive 100.0f spike should be smoothed by SGolay and caught by the Outlier filter
  BOOST_CHECK_LT(dstData[5], 100.0f);
}

BOOST_AUTO_TEST_SUITE_END()
