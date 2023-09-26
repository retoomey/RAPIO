#pragma once

// #include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOProgram.h>
#include <rError.h>
#include <rEventLoop.h>
#include <rRAPIOAlgorithm.h> // patch temp

#include <rHeartbeat.h>
#include <rWebServer.h>

namespace rapio {
/** A program plugin interface.
 * The design idea here is to encapsulate abilities like heartbeat, etc. within
 * its own module along with its related member fields.
 *
 * Plugins currently call back to the program using virtual plug methods in RAPIOProgram
 *
 * @author Robert Toomey
 */
class RAPIOPlugin : public Utility {
public:

  /** Declare plugin with unique name */
  RAPIOPlugin(const std::string& name) : myName(name), myActive(false){ }

  /** Get name of plugin */
  std::string
  getName()
  {
    return myName;
  }

  /** Return if plugin is active */
  virtual bool
  isActive()
  {
    return myActive;
  }

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

protected:
  /** Name of plug, typically related to command line such as 'sync', 'web' */
  std::string myName;

  /** Simple flag telling if plugin is active/running, etc. */
  bool myActive;
};

/** Subclass of plugin adding a heartbeat ability */
class PluginHeartbeat : public RAPIOPlugin {
public:

  /** Create a heartbeat plugin */
  PluginHeartbeat(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "sync")
  {
    owner->addPlugin(new PluginHeartbeat(name));
    return true;
  }

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override
  {
    o.optional(myName, "", "Sync data option. Cron format style algorithm heartbeat.");
    o.addGroup(myName, "TIME");
  }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override
  {
    o.addAdvancedHelp(myName,
      "Sends heartbeat to program.  Note if your program lags you may miss heartbeat.  For example, you have a 1 min heartbeat but take 2 mins to calculate/write.  You will get the next heartbeat window.  The format is a 6 star second supported cronlist, such as '*/10 * * * * *' for every 10 seconds.");
  }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override
  {
    myCronList = o.getString(myName);
  }

  /** Execute/run the plugin */
  virtual void
  execute(RAPIOProgram * caller) override
  {
    // Add Heartbeat (if wanted)
    std::shared_ptr<Heartbeat> heart = nullptr;

    if (!myCronList.empty()) { // None required, don't make it
      LogInfo("Attempting to create a heartbeat for " << myCronList << "\n");

      heart = std::make_shared<Heartbeat>((RAPIOAlgorithm *) (caller), 1000);
      if (!heart->setCronList(myCronList)) {
        LogSevere("Bad format for -" << myName << " string, aborting\n");
        exit(1);
      }
      EventLoop::addEventHandler(heart);
      myActive = true;
    }
  }

protected:

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;
};

/** Subclass of plugin doing webserver ability */
class PluginWebserver : public RAPIOPlugin {
public:

  /** Create a web server plugin */
  PluginWebserver(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "web")
  {
    owner->addPlugin(new PluginWebserver(name));
    return true;
  }

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override
  {
    o.optional(myName,
      "off",
      "Web server ability for REST pull algorithms. Use a number to assign a port.");
    o.setHidden(myName);
    o.addGroup("web", "I/O");
  }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override
  {
    o.addAdvancedHelp(myName,
      "Allows you to run the algorithm as a web server.  This will call processWebMessage within your algorithm.  -web=8080 runs your server on http://localhost:8080.");
  }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override
  {
    myWebServerMode = o.getString(myName);
  }

  /** Execute/run the plugin */
  virtual void
  execute(RAPIOProgram * caller) override
  {
    const bool wantWeb = (myWebServerMode != "off");

    myActive = wantWeb;
    if (wantWeb) {
      WebServer::startWebServer(myWebServerMode, caller);
    }
  }

protected:

  /** The mode of web server, if any */
  std::string myWebServerMode;
};
}
