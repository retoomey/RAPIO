#include "rHeartbeat.h"

#include "rError.h"
#include "rStrings.h"
#include "rRAPIOProgram.h"

using namespace rapio;
using namespace std;

Heartbeat::Heartbeat(RAPIOProgram * prog) : myProgram(prog),
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
}

void
Heartbeat::checkForPulse()
{
  if (!myParsed) {
    return;
  }

  // This is either now for realtime, or latest record
  // for archive mode
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
      myProgram->processHeartbeat(n, myLastPulseTime);
    }
    myLastPulseTime = pulse;
    // LogSevere("PULSE: " << n << " --- " << pulse << "\n");
  }
} // Heartbeat::checkForPulse

HeartbeatTimer::HeartbeatTimer(std::shared_ptr<Heartbeat> h, size_t milliseconds) : EventTimer(milliseconds,
    "HeartbeatTimer"),
  myHeartbeat(h)
{ }

void
HeartbeatTimer::action()
{
  if (myHeartbeat) {
    myHeartbeat->checkForPulse();
  }
} // HeartbeatTimer::action
