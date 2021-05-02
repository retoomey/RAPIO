#include "rWebServer.h"

#include "rError.h"
#include "rEventLoop.h"

#include <memory>

// We can do a local include here since this is a header only library
// if installed, algorithms won't have this header, that's ok
// right now we'll provide our own interface
#include "../webserver/server_http.hpp"

using namespace rapio;

std::shared_ptr<WebMessageQueue> rapio::WebMessageQueue::theWebMessageQueue;

WebMessageQueue::WebMessageQueue(
  RAPIOAlgorithm * alg
) : EventTimer(0, "WebMessageQueue") // Run me as fast as you can
{
  myAlg = alg;
}

void
WebMessageQueue::addRecord(std::shared_ptr<WebMessage> record)
{
  // Have to lock since we share with web thread
  myQueueLock.lock();
  myQueue.push(record);
  myQueueLock.unlock();
}

void
WebMessageQueue::action()
{
  myQueueLock.lock();
  if (!myQueue.empty()) {
    auto r = myQueue.front();
    myQueue.pop();

    // Process the web message within the algorithm worker thread
    myAlg->processWebMessage(r);

    // We promised to fill in the data when handled, the web thread is waiting
    // for us.
    r->result.set_value(true);
  }
  myQueueLock.unlock();
}

void
WebServer::startWebServer(const std::string& params)
{
  using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

  // HTTP-server at give port using 1 thread
  // Unless you do more heavy non-threaded processing in the resources,
  // 1 thread is usually faster than several threads
  // I think for now at least we'll limit RAPIO to a single webthread
  // and one main worker for simplicity.  If this ability becomes super
  // useful we could expand on it
  int port;
  try{
    port = std::stoi(params);
  }catch (const std::exception& e) {
    LogSevere("Couldn't get port from '" << params << "', exiting.\n");
    exit(1);
  }
  HttpServer server;
  server.config.port = port;

  // Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
  // Lambas are cool..but not the greatest when it comes to readability and code
  // organization.  Sigh I'm probably gonna start using them
  server.default_resource["GET"] = [](
    std::shared_ptr<HttpServer::Response> response,
    std::shared_ptr<HttpServer::Request> request)
    {
      // Create web message record for the worker queue, this
      // contains a promise that the worker thread will forfill.
      // Copy fields of interest to avoid algorithms needing the library headers.
      // request->remote_endpoint().address().to_string()
      // request->remote_endpoint().port();
      // request->method (for now we're just gonna do GET)
      // request->path
      // request->http_version;
      // request->header (map of pairs)
      auto query_fields = request->parse_query_string();

      // Copy just in case they change their type later
      std::map<std::string, std::string> theFields;
      for (auto f:query_fields) {
        theFields[f.first] = f.second;
      }
      auto web = std::make_shared<WebMessage>(request->path, theFields);
      WebMessageQueue::theWebMessageQueue->addRecord(web);
      auto fut = web->result.get_future();

      // FIXME: We could do a proper timeout here and handle accordingly I think..
      try{
        bool result = fut.get(); // wait on the algorithm thread to process this

        // We've been waiting on this WebMessage, dump its message for now.
        // FIXME: I'm gonna want file/MIME support here, etc.
        // First pass just doing text..lots of glue work left to do.
        if (result) {
          std::stringstream stream;
          stream << web->message;
          response->write(stream);
        } else {
          // Algorithm basically reported an error maybe?
        }
      }catch (const std::exception& e) {
        LogSevere("Error handling web request: " << e.what() << "\n");
        // FIXME: do http error codes with library properly
      }
    };

  // Start server and receive assigned port when server is listening for requests
  promise<unsigned short> server_port;
  thread server_thread([&server, &server_port](){
    // Start server
    server.start([&server_port](unsigned short port){
      server_port.set_value(port);
    });
  });
  LogInfo("Algorithm Web Server Initialized on port: " << server_port.get_future().get() << "\n");

  // Wait until server shutsdown
  // Do it here for the moment else the other thread context dies.
  EventLoop::doEventLoop();

  // server_thread.join(); // if we keep this context alive don't need this since exit will kill us
} // WebServer::startWebServer
