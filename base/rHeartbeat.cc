#include "rHeartbeat.h"

#include "rError.h"
#include "rStrings.h"
#include "rRAPIOProgram.h"

using namespace rapio;
using namespace std;

Heartbeat::Heartbeat(RAPIOProgram * prog) : myProgram(prog),
  myFirstPulse(true), myParsed(false)
{ }

bool
Heartbeat::setCronList(const std::string& cronlist)
{
  const char * err = NULL;

  myParsed = false; // In case we call it again, disable current one
  try {
    // croncpp throws on bad syntax instead of using C-style error pointers
    myCronExpr = cron::make_cron(cronlist);
    myParsed   = true;
    return true;
  } catch (const cron::bad_cronexpr& e) {
    fLogSevere("Failed to parse cron expression: '{}' Err: {}", cronlist, e.what());
    return false;
  }
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
  time_t next = cron::cron_next(myCronExpr, cur);
  Time pulse(next);

  if (pulse != myLastPulseTime) {
    if (myFirstPulse) {
      // ignore first one, it's 0
      myFirstPulse = false;
    } else {
      myProgram->processHeartbeat(n, myLastPulseTime);
    }
    myLastPulseTime = pulse;
    // fLogSevere("PULSE: {} --- {}", n, pulse);
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
