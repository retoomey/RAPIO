#pragma once
#include <rRAPIOProgram.h>
#include <string>
#include <memory>
#include <vector>
#include <sys/types.h>
#include <rEventLoop.h>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/asio.hpp>
BOOST_WRAP_POP

namespace rapio {
/** Experimental.  Wrap ldmd as a supervisor process, similar to how
 * modern nginx/apache monitor the connection process.
 * Our rdm is still using the ancient C style fork.  Probably should
 * switch to a thread model at some point and harden it with
 * exception catching (like modern Apache/NGINX) */
class rNotify : public RAPIOProgram {
public:
  rNotify();
  virtual
  ~rNotify();
  virtual void
  declarePlugins() override;
  virtual void
  declareOptions(RAPIOOptions& o) override;
  virtual void
  processOptions(RAPIOOptions& o) override;
  virtual void
  execute() override;
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

private:
  void
  startServer();
  void
  startLogCapture();
  void
  handleChildExit(int status); // Added for unified lifecycle tracking
  void
  provisionDefaultConfig();
  void
  verifyQueue();

  std::string myRDMPort{ "6388" };
  std::string myQueuePath;
  std::string myConfPath;
  std::string myBinDir{ "/usr/local/bin" };
  pid_t myWatchedPID{ -1 };
  bool myShuttingDown{ false };
  bool myServerReady{ false };
  std::unique_ptr<boost::asio::posix::stream_descriptor> myLogPipe;
  boost::asio::streambuf myLogBuffer;

  std::chrono::steady_clock::time_point myLastTime;
  boost::asio::steady_timer myRespawnTime{ EventLoop::io_context() };
};
}
