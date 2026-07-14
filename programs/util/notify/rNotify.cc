#include "rNotify.h"
#include "rConfig.h"
#include "rOS.h"
#include <rColorTerm.h>
#include <rEventLoop.h>
#include <rPluginHeartbeat.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <istream>
#include <sys/wait.h>
#include <unistd.h>
#include "ProcessUtil.h"
#include <iostream>
#include <fstream>

using namespace rapio;

namespace rapio {
constexpr const char * BIN_RDM       = "ldmd";
constexpr const char * BIN_RPQCREATE = "pqcreate";
}

rNotify::rNotify() : RAPIOProgram("rNotify"){ }

rNotify::~rNotify()
{
  fLogInfo("Initiating rNotify supervisor shutdown...");
  myShuttingDown = true;

  if (myLogPipe) {
    boost::system::error_code ec;
    myLogPipe->close(ec);
  }

  if (myWatchedPID > 0) {
    fLogInfo("Sending SIGTERM to managed {} process (PID: {})", BIN_RDM, myWatchedPID);
    // Sending SIGTERM allows the engine's destructor to catch it,
    // run StopEngine(), and cleanly execute its own kill(0, SIGTERM)
    // to reap its process group tree.
    ::kill(myWatchedPID, SIGTERM);

    int wstatus;
    for (int i = 0; i < 20; ++i) {
      if (waitpid(myWatchedPID, &wstatus, WNOHANG) > 0) { break; }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (waitpid(myWatchedPID, &wstatus, WNOHANG) == 0) {
      fLogSevere("{} (PID: {}) did not shut down in time. Sending SIGKILL.", BIN_RDM, myWatchedPID);
      ::kill(myWatchedPID, SIGKILL);
      waitpid(myWatchedPID, &wstatus, 0);
    }
  }
  fLogInfo("rNotify shutdown complete.");
}

void
rNotify::declarePlugins()
{
  PluginHeartbeat::declare(this, "sync");
}

void
rNotify::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO Notifier Supervisor");
  o.setAuthors("Robert Toomey");
  o.optional("P", "9388", "The port number for connections");
  o.optional("q", "", "Product-queue pathname");
  o.optional("c", "./ldmd.conf", "Configuration file");
  o.optional("B", "./", "Directory containing the executable");
  o.setDefaultValue("sync", "*/10 * * * * *"); // Active evaluation check every 10 seconds
}

void
rNotify::processOptions(RAPIOOptions& o)
{
  myRDMPort   = o.getString("P");
  myQueuePath = o.getString("q");
  myConfPath  = o.getString("c");
  myBinDir = o.getString("B");
}

void
rNotify::execute()
{
  fLogInfo("{}{}--- Starting rNotify Supervisor Engine ---{}", ColorTerm::green(), ColorTerm::bold(),
    ColorTerm::reset());
  startServer();
  fLogInfo("Supervisor active. RAPIO Event Loop taking over...");

  EventLoop::doEventLoop(); // Block and yield execution control to Asio
}

void
rNotify::verifyQueue()
{
  // If the user didn't specify a queue, assume the default and SAVE it
  if (myQueuePath.empty()) {
    myQueuePath = "var/queues/ldm.pq";
  }

  if (!rapio::OS::isRegularFile(myQueuePath)) {
    fLogInfo("Product queue '{}' not found. Provisioning a default 500MB queue...", myQueuePath);

    // Ensure the directory exists
    std::string dir  = "var/queues";
    size_t lastSlash = myQueuePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
      dir = myQueuePath.substr(0, lastSlash);
    }
    rapio::OS::ensureDirectory(dir);

    // Build the setup  command
    std::string binPath = myBinDir + "/" + BIN_RPQCREATE;
    std::vector<std::string> args = { binPath, "-c", "-s", "500M", "-q", myQueuePath };
    std::vector<char *> c_args;
    for (auto& s : args) {
      c_args.push_back(const_cast<char *>(s.c_str()));
    }
    c_args.push_back(nullptr);

    rdm::os::ExecParams params;
    params.argv         = c_args.data();
    params.resetSignals = true;

    fLogInfo("Executing: {} -c -s 500M -q {}", binPath, myQueuePath);
    pid_t childPid = rdm::os::ForkAndExec(params);

    if (childPid > 0) {
      int wstatus;
      waitpid(childPid, &wstatus, 0);
      if (WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) == 0)) {
        fLogInfo("Successfully created product queue at {}", myQueuePath);
      } else {
        fLogSevere("Failed to provision product queue (Exit Status {}). The engine may fail to start.",
          WEXITSTATUS(wstatus));
      }
    } else {
      fLogSevere("Failed to fork {} process!", BIN_RPQCREATE);
    }
  }
} // rNotify::verifyQueue

void
rNotify::startServer()
{
  if (myShuttingDown) { return; }

  if (!myConfPath.empty() && !OS::isRegularFile(myConfPath)) {
    fLogInfo("Configuration file '{}' not found. Attempting automatic provisioning...", myConfPath);
    provisionDefaultConfig();
  }

  verifyQueue();

  myLastTime = std::chrono::steady_clock::now();
  myServerReady   = false; // Reset readiness state on spawn

  int pipefd[2];

  if (pipe(pipefd) != 0) {
    fLogSevere("Failed to create log pipe for {}: {}", BIN_RDM, strerror(errno));
    return;
  }

  std::string binPath = myBinDir + "/" + BIN_RDM;

  // We pass "-l -" to route the internal spdlog output to stdout so we can pipe it.
  // The engine naturally stays in the foreground now because we removed the DONTFORK macro.
  std::vector<std::string> args = { binPath, "-l", "-", "-P", myRDMPort };

  if (!myQueuePath.empty()) {
    args.push_back("-q");
    args.push_back(myQueuePath);
  }
  if (!myConfPath.empty()) {
    args.push_back(myConfPath);
  }

  std::vector<char *> c_args;

  for (auto& s : args) {
    c_args.push_back(const_cast<char *>(s.c_str()));
  }
  c_args.push_back(nullptr);

  rdm::os::ExecParams params;

  params.argv         = c_args.data();
  params.stdoutFd     = pipefd[1];
  params.stderrFd     = pipefd[1];
  params.resetSignals = true;
  params.setPgid      = true; // Hard POSIX boundary. Protects RAPIO from the daemon's group kill!

  myWatchedPID = rdm::os::ForkAndExec(params);
  close(pipefd[1]);

  if (myWatchedPID < 0) {
    fLogSevere("Failed to spawn {} process!", BIN_RDM);
    close(pipefd[0]);
    return;
  }

  fLogInfo("Successfully spawned managed {} process (PID: {})", BIN_RDM, myWatchedPID);
  myLogPipe = std::make_unique<boost::asio::posix::stream_descriptor>(EventLoop::io_context(), pipefd[0]);
  startLogCapture();
} // rNotify::startServer

void
rNotify::provisionDefaultConfig()
{
  std::string dir  = "etc";
  size_t lastSlash = myConfPath.find_last_of('/');

  if (lastSlash != std::string::npos) {
    dir = myConfPath.substr(0, lastSlash);
  }
  rapio::OS::ensureDirectory(dir);

  std::ofstream out(myConfPath);

  if (!out.is_open()) {
    fLogSevere("Unable to provision default config at '{}'. Child process will likely fail.", myConfPath);
    return;
  }

  auto doc = Config::huntXML("ldm/default_rules.xml");

  if (doc != nullptr) {
    fLogInfo("Extracting configurations from RAPIO global search paths...");
    try {
      auto rules = doc->getTree()->getChildren("rule");
      for (const auto& rule : rules) {
        out << rule.get(std::string("")) << "\n";
      }
      out.close();
      return;
    } catch (...) {
      fLogSevere("Error parsing global XML rules. Falling back to hardcoded safe mode.");
    }
  }

  fLogInfo("Provisioning minimal hardcoded loopback configurations.");
  out << "#################################################################\n";
  out << "# rNotify Auto-Generated Minimal Safe-Mode Configuration\n";
  out << "#################################################################\n\n";
  out << "ALLOW\tANY\t^127\\.0\\.0\\.1$|^localhost$\t.*\n";
  out << "ACCEPT\tANY\t.*\t^127\\.0\\.0\\.1$|^localhost$\n\n";
  out << "# Operators can safely override this file with native LDM rules.\n";

  out.close();
} // rNotify::provisionDefaultConfig

void
rNotify::startLogCapture()
{
  if (!myLogPipe || myShuttingDown) { return; }

  boost::asio::async_read_until(*myLogPipe, myLogBuffer, '\n',
    [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (myShuttingDown) { return; }

    std::istream is(&myLogBuffer);
    std::string line;
    while (std::getline(is, line)) {
      if (!line.empty()) {
        // INTERCEPT THE READINESS TOKEN
        if (line.find("[RDM_READY]") != std::string::npos) {
          fLogInfo("{}{}Supervisor caught readiness token. Daemon is bound and active!{}", ColorTerm::green(),
          ColorTerm::bold(), ColorTerm::reset());
          myServerReady = true; // Signals upstream components that the port is open
        } else {
          // Standard log pass-through
          fLogInfo("[{}-{}] {}", BIN_RDM, myWatchedPID > 0 ? myWatchedPID : 0, line);
        }
      }
    }

    if (!ec) {
      startLogCapture();
    } else {
      if (ec == boost::asio::error::eof) {
        fLogDebug("Log pipe reached EOF naturally for {} PID {}", BIN_RDM, myWatchedPID);
      } else if (ec != boost::asio::error::operation_aborted) {
        fLogDebug("Log pipe closed unexpectedly: {}", ec.message());
      }

      if (myLogPipe) {
        boost::system::error_code close_ec;
        myLogPipe->close(close_ec);
        myLogPipe.reset();
      }

      if (myWatchedPID > 0) {
        int status = 0;
        if (::waitpid(myWatchedPID, &status, WNOHANG) > 0) {
          handleChildExit(status);

          auto now    = std::chrono::steady_clock::now();
          auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - myLastTime).count();

          if (uptime < 2) {
            fLogSevere("Managed daemon is thrashing (died instantly in {}s). Enforcing 3-second backoff...", uptime);
            myRespawnTime.expires_after(std::chrono::seconds(3));
            myRespawnTime.async_wait([this](const boost::system::error_code& timer_ec) {
              if (!timer_ec && !myShuttingDown) {
                fLogInfo("Uptime backoff complete. Attempting daemon respawn...");
                startServer();
              }
            });
          } else {
            fLogInfo("Hot-respawning managed daemon from stream shutdown trigger...");
            startServer();
          }
        }
      }
    }
  });
} // rNotify::startLogCapture

void
rNotify::processHeartbeat(const Time& n, const Time& p)
{
  if (myShuttingDown) { return; }

  if (myWatchedPID < 0) {
    if (!myLogPipe) {
      fLogInfo("Supervisor respawning managed daemon via heartbeat monitor...");
      startServer();
    }
    return;
  }

  int status = 0;
  pid_t wpid = ::waitpid(myWatchedPID, &status, WNOHANG);

  if (wpid == myWatchedPID) {
    handleChildExit(status);
    // Let the async stream descriptor empty the buffer naturally.
    // Once it encounters EOF, the read handler will hot-restart the process safely.
  }
}

void
rNotify::handleChildExit(int status)
{
  if (WIFSIGNALED(status)) {
    fLogSevere("Managed {} (PID {}) crashed via signal {}!", BIN_RDM, myWatchedPID, WTERMSIG(status));
  } else if (WIFEXITED(status)) {
    fLogInfo("Managed {} (PID {}) exited cleanly with status {}.", BIN_RDM, myWatchedPID, WEXITSTATUS(status));
  }
  myWatchedPID = -1;
  myServerReady = false; // Reset readiness on exit
}

int
main(int argc, char * argv[])
{
  rapio::rNotify alg;

  alg.executeFromArgs(argc, argv);
  return 0;
}
