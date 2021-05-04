#include "rWebServer.h"

#include "rError.h"
#include "rEventLoop.h"

#include <memory>

// We can do a local include here since this is a header only library
// if installed, algorithms won't have this header, that's ok
// right now we'll provide our own interface
#include "../webserver/server_http.hpp"

using namespace rapio;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

std::shared_ptr<WebMessageQueue> rapio::WebMessageQueue::theWebMessageQueue;

// Copying the example given in simple web server
// FIXME: This could just be a method in the WebServer
class FileServer : public Utility {
public:

  static void
  read_and_send(const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs)
  {
    // Read and send 128 KB at a time
    static vector<char> buffer(131072); // Safe when server is running on one thread
    streamsize read_length;

    if ((read_length = ifs->read(&buffer[0], static_cast<streamsize>(buffer.size())).gcount()) > 0) {
      response->write(&buffer[0], read_length);
      if (read_length == static_cast<streamsize>(buffer.size())) {
        response->send([response, ifs](const SimpleWeb::error_code &ec){
          if (!ec) {
            read_and_send(response, ifs); // FIXME: I kinda hate tail end recursion...just loop and save your stack, lol
          } else {
            std::cerr << "Connection interrupted" << endl;
          }
        });
      }
    }
  }
};

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
          // How to send an error properly...
          // response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what());

          if (web->isFile()) {
            // Simple attempt to send a file
            // static vector<char> buffer(131072); // Safe when server is running on one thread
            auto ifs = std::make_shared<ifstream>(); // cute trick, auto close
            ifs->open(web->getFile(), ifstream::in | ios::binary | ios::ate);
            if (*ifs) {
              // Ok get the length of the file we're gonna send, let the client know
              auto length = ifs->tellg();
              ifs->seekg(0, ios::beg);
              SimpleWeb::CaseInsensitiveMultimap header;
              header.emplace("Content-Length", to_string(length));
              response->write(header);

              FileServer::read_and_send(response, ifs);
            } else {
              LogSevere("Failed to open filename " << web->getFile() << "\n");
              web->setMessage("Failed to open filename " + web->getFile());
              std::stringstream stream;
              stream << web->getMessage();
              response->write(stream);
            }
          } else {
            LogSevere("Not 'file', sending text\n");
            std::stringstream stream;
            stream << web->getMessage();
            response->write(stream);
          }
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
