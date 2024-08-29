#include "rPluginWebserver.h"

#include <rRAPIOAlgorithm.h>
#include <rWebServer.h>

using namespace rapio;
using namespace std;

bool
PluginWebserver::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginWebserver(name));
  return true;
}

void
PluginWebserver::declareOptions(RAPIOOptions& o)
{
  o.optional(myName,
    "off",
    "Web server ability for REST pull algorithms. Use a number to assign a port.");
  o.setHidden(myName);
  o.addGroup(myName, "I/O");
}

void
PluginWebserver::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName,
    "Allows you to run the algorithm as a web server.  This will call processWebMessage within your algorithm.  -" + myName
    + "=8080 runs your server on http://localhost:8080.");
}

void
PluginWebserver::processOptions(RAPIOOptions& o)
{
  myWebServerMode = o.getString(myName);
}

void
PluginWebserver::execute(RAPIOProgram * caller)
{
  const bool wantWeb = (myWebServerMode != "off");

  myActive = wantWeb;
  if (wantWeb) {
    WebServer::startWebServer(myWebServerMode, caller);
  }
}
