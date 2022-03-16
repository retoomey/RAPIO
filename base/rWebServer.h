#pragma once

#include <rUtility.h>
#include <rEventTimer.h>
#include <rRAPIOAlgorithm.h>
#include <future>
#include <mutex>

// We can do a local include here since this is a header only library
// if installed, algorithms won't have this header, that's ok
// right now we'll provide our own interface
#include "../webserver/server_http.hpp"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace rapio {
class WebMessage : public Utility
{
public:
  WebMessage(const std::string& path, const std::map<std::string, std::string>& map)
    : myPath(path), myMap(map), message(""), file(""){ }

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
  setFile(const std::string& f)
  {
    file    = f;
    message = "file"; // hack for moment, need http codes too
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

protected:

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
    RAPIOAlgorithm * alg
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

  /** The algorithm we send records to */
  RAPIOAlgorithm * myAlg;

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
  startWebServer(const std::string& params, RAPIOAlgorithm * alg);

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
