#pragma once

// #include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOProgram.h>
#include <rError.h>
#include <rEventLoop.h>
#include <rRAPIOAlgorithm.h> // patch temp

namespace rapio {
/** A program plugin interface.
 * The design idea here is to encapsulate abilities like heartbeat, etc. within
 * its own module along with its related member fields.
 *
 * Plugins call back to the algorithm, how to handle this?  If it's too
 * generic could be confusing, if not general enough we lose the point of the plugin.
 *
 * @author Robert Toomey
 */
class RAPIOPlugin : public Utility {
public:

  /** Declare options used */
  virtual void
  declareOptions(RAPIOOptions& o)
  { }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o)
  { }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o)
  { }

  /** Setup/run the plugin */
  virtual void
  execute(RAPIOProgram * caller)
  { }
};

/** Subclass of plugin adding a heartbeat ability */
class PluginHeartbeat : public RAPIOPlugin {
public:

  #define PLUGINHEART "sync"

  /** Declare plugin.  Humm could be a create right? */
  static bool
  declare(RAPIOProgram * owner)
  {
    owner->addPlugin(new PluginHeartbeat()); // Humm exposes unness
    return true;                             // success
  }

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override
  {
    o.optional(PLUGINHEART, "", "Sync data option. Cron format style algorithm heartbeat.");
    o.addGroup(PLUGINHEART, "TIME");
  }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override
  {
    o.addAdvancedHelp(PLUGINHEART,
      "In daemon/realtime sends heartbeat to algorithm.  Note if your algorithm lags you may miss heartbeat.  For example, you have a 1 min heartbeat but take 2 mins to calculate/write.  You will get the next heartbeat window.  The format is a 6 star second supported cronlist, such as '*/10 * * * * *' for every 10 seconds.");
  }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override
  {
    myCronList = o.getString(PLUGINHEART);
  }

  virtual void
  execute(RAPIOProgram * caller) override
  {
    // Add Heartbeat (if wanted)
    std::shared_ptr<Heartbeat> heart = nullptr;

    if (!myCronList.empty()) { // None required, don't make it
      LogInfo("Attempting to create heartbeat for " << myCronList << "\n");

      // FIXME: humm should be more generic timer class without relying on RAPIOAlgorithm?
      // If it were more generic wouldn't need Heartbeat subclass here at all
      heart = std::make_shared<Heartbeat>((RAPIOAlgorithm *) (caller), 1000);
      if (!heart->setCronList(myCronList)) {
        LogSevere("Bad format for -sync string, aborting\n");
        exit(1);
      }
      EventLoop::addEventHandler(heart);
    }
  }

protected:

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;
};

/** Subclass of plugin doing webserver ability */
class PluginWebserver : public RAPIOPlugin {
public:
  // Ok so the hooks for command line stuff...

  // void options(RAPIOOptions& o){
  void
  options()
  {
    // o.optional("web",
    //   "off",
    //   "Web server ability for REST pull algorithms. Use a number to assign a port.");
    // o.setHidden("web");
  }

  // void addPostLoadedHelp(RAPIOOptions& o)
  void
  addPostLoadedHelp()
  {
    //  o.addAdvancedHelp("web",
    //    "Allows you to run the algorithm as a web server.  This will call processWebMessage within your algorithm.  -web=8080 runs your server on http://localhost:8080.");
  }

  void
  processOptions()
  {
    // myWebServerMode = o.getString("web");
  }

  void
  execute()
  {
    /*
     * // Launch event loop, either along with web server, or solo
     * const bool wantWeb = (myWebServerMode != "off");
     *
     * myWebServerOn = wantWeb;
     * if (wantWeb) {
     *  WebServer::startWebServer(myWebServerMode, this);
     * }
     */
  }
};
}
