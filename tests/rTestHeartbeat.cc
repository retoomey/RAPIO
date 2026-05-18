#include "rBOOSTTest.h"
#include "rHeartbeat.h"
#include "rRAPIOProgram.h"

using namespace rapio;

// 1. Create a dummy program to intercept and count heartbeat triggers
class MockHeartbeatProgram : public RAPIOProgram {
public:
  int pulseCount = 0;

  MockHeartbeatProgram() : RAPIOProgram("MockHeartbeatProgram"){ }

  // Override the callback so we know when the heartbeat fires
  void
  processHeartbeat(const Time& n, const Time& p) override
  {
    pulseCount++;
  }
};

BOOST_AUTO_TEST_SUITE(HEARTBEAT_TESTS)

BOOST_AUTO_TEST_CASE(TEST_CRON_PARSING_VALID)
{
  MockHeartbeatProgram mockProg;
  Heartbeat hb(&mockProg);

  // Test standard 6-part cron formats used in your system
  BOOST_CHECK_MESSAGE(hb.setCronList("*/10 * * * * *"), "Should parse: Every 10 seconds");
  BOOST_CHECK_MESSAGE(hb.setCronList("0 */2 * * * *"), "Should parse: Every 2 minutes at 0th second");
  BOOST_CHECK_MESSAGE(hb.setCronList("0 0 * * * *"), "Should parse: Top of every hour");
  BOOST_CHECK_MESSAGE(hb.setCronList("* * * * * *"), "Should parse: Every second");
}

BOOST_AUTO_TEST_CASE(TEST_CRON_PARSING_INVALID)
{
  MockHeartbeatProgram mockProg;
  Heartbeat hb(&mockProg);

  // Test that the parser gracefully rejects garbage
  BOOST_CHECK_MESSAGE(!hb.setCronList("garbage string"), "Should reject random text");
  BOOST_CHECK_MESSAGE(!hb.setCronList("61 * * * * *"), "Should reject invalid seconds (61)");
  BOOST_CHECK_MESSAGE(!hb.setCronList("0 0 25 * * *"), "Should reject invalid hours (25)");
  BOOST_CHECK_MESSAGE(!hb.setCronList(
      "*/10 * * * *"), "Should reject standard 5-part cron (requires 6-part with seconds)");
}

BOOST_AUTO_TEST_CASE(TEST_PULSE_EXECUTION_STATE)
{
  MockHeartbeatProgram mockProg;
  Heartbeat hb(&mockProg);

  // Set a valid cron
  BOOST_REQUIRE(hb.setCronList("*/10 * * * * *"));

  // The first call to checkForPulse() initializes the baseline time
  // but should NOT trigger the program's processHeartbeat yet.
  hb.checkForPulse();
  BOOST_CHECK_EQUAL(mockProg.pulseCount, 0);

  // Note: To thoroughly test subsequent triggers, you would normally need to mock
  // the system clock (Time::CurrentTime). For this refactor, just ensuring the
  // parser state and initial pulse behavior remain identical is the main priority.
}

BOOST_AUTO_TEST_SUITE_END()
