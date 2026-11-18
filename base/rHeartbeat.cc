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

  if (pulse != myNextPulseTime) {
    if (myFirstPulse) {
      // ignore first one, it's 0
      myFirstPulse    = false;
      myPrevPulseTime = n;
    } else {
      myProgram->processHeartbeat(n, myPrevPulseTime);
      myPrevPulseTime = myNextPulseTime;
    }
    myNextPulseTime = pulse;
    // fLogSevere("PULSE: {} --- {}", n, pulse);
  }
} // Heartbeat::checkForPulse

void
Heartbeat::advanceToTime(const Time& recordTime)
{
  if (!myParsed) {
    return;
  }

  time_t recSec = recordTime.getSecondsSinceEpoch();

  if (myFirstPulse) {
    myFirstPulse    = false;
    myPrevPulseTime = recordTime;
    // Floor to the minute/cron boundary before or at recordTime?
    // cron_next(recSec) finds the first cron tick strictly > recSec.
    // Subtracting 1 second ensures that if the first record is at 22:04:00,
    // the first scheduled pulse is 22:04:00 rather than skipping to 22:06:00.
    myNextPulseTime = Time(cron::cron_next(myCronExpr, recSec > 0 ? recSec - 1 : 0));
    return;
  }

  // Step through every scheduled cron boundary strictly before recordTime
  // BEFORE the new record is ingested.
  while (myNextPulseTime < recordTime) {
    Time firedPulse = myNextPulseTime;
    Time::setLatestDataTime(firedPulse);
    myProgram->processHeartbeat(firedPulse, myPrevPulseTime);
    myPrevPulseTime = firedPulse;
    myNextPulseTime = Time(cron::cron_next(myCronExpr, firedPulse.getSecondsSinceEpoch()));
  }
} // Heartbeat::advanceToTime

void
Heartbeat::finalizeArchive()
{
  if (!myParsed || myFirstPulse) {
    return;
  }

  // Trigger one final heartbeat at the end of the archive to flush
  // any remaining data accumulated after the last cron boundary.
  Time firedPulse = myNextPulseTime;

  Time::setLatestDataTime(firedPulse);
  myProgram->processHeartbeat(firedPulse, myPrevPulseTime);
  myPrevPulseTime = firedPulse;
  myNextPulseTime = Time(cron::cron_next(myCronExpr, firedPulse.getSecondsSinceEpoch()));
} // Heartbeat::finalizeArchive

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
