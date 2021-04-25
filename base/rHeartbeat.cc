#include "rHeartbeat.h"

#include "rError.h"
#include "rStrings.h"
#include "rRAPIOAlgorithm.h"

using namespace rapio;
using namespace std;

Heartbeat::Heartbeat(RAPIOAlgorithm * alg, size_t milliseconds) : EventTimer(milliseconds, "Heartbeat"), myAlg(alg),
  myFirstPulse(true), myParsed(false), myCronExpr(cron_expr())
{ }

bool
Heartbeat::setCronList(const std::string& cronlist)
{
  const char * err = NULL;

  myParsed = false; // In case we call it again, disable current one
  cron_parse_expr(cronlist.c_str(), &myCronExpr, &err);
  if (err) {
    LogSevere("Failed to parse cron expression: '" << cronlist << "' Err: " << err << "\n");
    return false;
  }
  myParsed = true;
  return true;
} // Heartbeat::parse

void
Heartbeat::action()
{
  if (!myParsed) {
    return;
  }

  // Get now
  Time n = Time::CurrentTime();

  // Get the next pulse using cron expr.
  // This will happen when we pass the old trigger
  // time_t cur = time(NULL); Let it grab current
  time_t cur  = n.getSecondsSinceEpoch();
  time_t next = cron_next(&myCronExpr, cur);
  Time pulse(next);

  if (pulse != myLastPulseTime) {
    if (myFirstPulse) {
      // ignore first one, it's 0
      myFirstPulse = false;
    } else {
      myAlg->handleTimedEvent(n, myLastPulseTime);
    }
    myLastPulseTime = pulse;
    // LogSevere("PULSE: "<<n << " --- " << pulse << "\n");
  }
} // Heartbeat::action
