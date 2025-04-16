#pragma once

#include <rUtility.h>
#include <rEventTimer.h>
#include <rRAPIOProgram.h>
#include <rStrings.h>
#include <future>
#include <mutex>
#include <queue>

// From the webserver folder 
#include "server_http.hpp"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace rapio {
class WebMessage : public Utility
{
  friend class WebServer;

public:

  WebMessage(const std::string& path, const std::map<std::string, std::string>& map)
    : myPath(path), myMap(map), message(""), file("")
  {
    setError(200); // Success by default
  }

  /** Promise set by main worker when work complete */
  std::promise<bool> result;

  // Keeping separate from our URL class for the moment
  // but 'maybe' merge later. IE the path and map of params

  /** Get the path */
  std::string
  getPath() const
  {
    return myPath;
  }

  /** Get the param map */
  const std::map<std::string, std::string>&
  getMap() const
  {
    return myMap;
  }

  /** Get the response message */
  std::string
  getMessage()
  {
    return message;
  }

  /** Get the header map */
  const std::map<std::string, std::string>&
  getHeaderMap() const
  {
    return myHeaders;
  }

  /** Get the file */
  std::string
  getFile()
  {
    return file;
  }

  /** Get the file */
  void
  setFile(const std::string& f, std::string type = "text/plain")
  {
    file    = f;
    message = "file";

    // Basic types we know of.
    // FIXME: Probably need a table general lookup here
    if (Strings::endsWith(file, ".png")) {
      type = "image/png";
    } else if (Strings::endsWith(file, ".html")) {
      type = "text/html";
    } else if (Strings::endsWith(file, ".css")) {
      type = "text/css";
    } else if (Strings::endsWith(file, ".js")) {
      type = "application/javascript";
    } else if (Strings::endsWith(file, ".wasm")) {
      type = "application/wasm";
    }

    setMessage("file", type);
  }

  /** Is this a file message */
  bool
  isFile()
  {
    return (message == "file");
  }

  /** Set the response message */
  void
  setMessage(const std::string& m, const std::string& type = "text/plain")
  {
    message = m;
    myHeaders.clear();
    myHeaders["Content-Type"] = type;
  }

  /** Get the http error value */
  size_t
  getError()
  {
    return myErrorNumber;
  }

  /** Get the http error string */
  std::string
  getErrorString()
  {
    // We know it exists in map so just return the string directly
    return SimpleWeb::status_code(myErrorInternal);
  }

  /** Set the response http error.  I don't want callers using
   * the SimpleWeb::StatusCode in case the underlying library changes,
   * so we map an error number to the enum.  The number is enough for us. */
  void
  setError(size_t error)
  {
    // The last enum value
    size_t enumSize = static_cast<size_t>(SimpleWeb::StatusCode::server_error_network_authentication_required);

    if (error > enumSize) {
      LogSevere("Trying to set http error value to " << error << " which is out of range\n");
      error = 0; // unknown
    }
    myErrorNumber   = error;
    myErrorInternal = static_cast<SimpleWeb::StatusCode>(error);
  }

  /** Set macros, any file will be parsed and macros replaced.  Basically a cheap
   * serverside ability */
  void
  addMacro(const std::string& macro, const std::string& value)
  {
    // FIXME: Not checking duplicates, first come first served
    myMacroKeys.push_back("MACRO(" + macro + ")"); // Make it full match
    myMacroValues.push_back(value);
  }

protected:

  /** Get the http error value */
  SimpleWeb::StatusCode
  getErrorInternal()
  {
    return myErrorInternal;
  }

  /** Path of the URL request */
  std::string myPath;

  /** Map of params to values */
  std::map<std::string, std::string> myMap;

  /** Map of header values */
  std::map<std::string, std::string> myHeaders;

  /** Message response for web server */
  std::string message;

  /** File path, if any */
  std::string file;

  /** Error status as a number, synced to SimpleWeb library */
  size_t myErrorNumber;

  /** Error status as SimpleWeb library, synced to error number */
  SimpleWeb::StatusCode myErrorInternal;

  /** Keys for macros */
  std::vector<std::string> myMacroKeys;

  /** Values for macros */
  std::vector<std::string> myMacroValues;
};

/** Web queue holds message from a webserver thread for the main thread to
 * process. We currently only allow one algorithm main thread to avoid
 * sync issues for students, etc.  This is typically 'difficult' to program
 * correctly.  In theory if we were running an algorithm that had lots
 * of concurrent calls we would be load balancing say with containers anyway.
 *
 * @author Robert Toomey
 */
class WebMessageQueue : public EventHandler
{
public:
  std::mutex myQueueLock;

  WebMessageQueue(
    RAPIOProgram * prog
  );

  /** Add given record to queue */
  void
  addRecord(std::shared_ptr<WebMessage> record);

  /** Fired action.  Usually process a record from queue */
  virtual void
  action() override;

  /** Global priority queue used to process Web messages  */
  static std::shared_ptr<WebMessageQueue> theWebMessageQueue;

protected:

  /** The program we send records to */
  RAPIOProgram * myProgram;

  /** Records.  We'll need a lock here I think.  Webserver will push, Main will pull. */
  std::queue<std::shared_ptr<rapio::WebMessage> > myQueue;
};

/** Main WebServer interface for passing to algorithms
 * using the webserver output option.
 * This is an interesting new design idea, not sure
 * if this will become a IODataType or what here.
 *
 * @author Robert Toomey
 */
class WebServer : public Utility {
public:

  /** Thread function for running web server */
  static void
  runningWebServer();

  /** Simple start web server */
  static void
  startWebServer(const std::string& params, RAPIOProgram * prog);

  /** Handle GET */
  static void
  handleGET(std::shared_ptr<HttpServer::Response> response,
    std::shared_ptr<HttpServer::Request>          request);

  /** Destroy web server */
  virtual ~WebServer(){ }

  /** The port used by webserver */
  static int port;
};
}
