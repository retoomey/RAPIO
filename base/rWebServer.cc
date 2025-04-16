#include "rWebServer.h"

#include "rError.h"
#include "rEventLoop.h"
#include "rOS.h"
#include "rStrings.h"

#include <memory>
#include <fstream>

using namespace rapio;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

int WebServer::port;

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
  RAPIOProgram * prog
) : EventHandler("WebMessageQueue"), myProgram(prog)
{ }

void
WebMessageQueue::addRecord(std::shared_ptr<WebMessage> record)
{
  // Have to lock since we share with web thread
  myQueueLock.lock();
  myQueue.push(record);
  myQueueLock.unlock();
  setReady();
}

void
WebMessageQueue::action()
{
  myQueueLock.lock();
  if (!myQueue.empty()) {
    auto r = myQueue.front();
    myQueue.pop();

    // Process the web message within the algorithm worker thread
    myProgram->processWebMessage(r);

    // We promised to fill in the data when handled, the web thread is waiting
    // for us.
    r->result.set_value(true);
  }
  myQueueLock.unlock();

  // process another web message when we can
  if (!myQueue.empty()) {
    setReady();
  }
}

void
WebServer::handleGET(std::shared_ptr<HttpServer::Response> response,
  std::shared_ptr<HttpServer::Request>                     request)
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
        const std::string path = web->getFile();
        bool sendText = false;

        if (OS::isDirectory(path)) {
          LogSevere("Directory forbidden: " + web->getFile());
          web->setMessage("Directory forbidden, though we could add this ability: " + web->getFile());
          web->setError(403);
          sendText = true;
        } else if (OS::isRegularFile(path)) {
          auto ifs = std::make_shared<ifstream>(); // cute trick, auto close
          const bool useMacros = (web->myMacroKeys.size() > 0);

          if (useMacros) {
            ifs->open(web->getFile());
          } else {
            ifs->open(web->getFile(), ifstream::in | ios::binary | ios::ate);
          }

          if ((*ifs) && ifs->is_open()) {
            // Add the headers
            SimpleWeb::CaseInsensitiveMultimap header;
            for (auto& a:web->getHeaderMap()) {
              header.emplace(a.first, a.second);
            }

            if (useMacros) {
              // Parse entire file and replace all macros
              std::stringstream stream;
              std::string line;
              while (std::getline(*ifs, line)) {
                stream << Strings::replaceGroup(line, web->myMacroKeys, web->myMacroValues) << "\n";
              }
              // Final write
              response->write(web->getErrorInternal(), stream, header);
            } else {
              // Ok get the length of the file we're gonna send, let the client know
              auto length = ifs->tellg();
              ifs->seekg(0, ios::beg);
              header.emplace("Content-Length", to_string(length));
              response->write(header);

              FileServer::read_and_send(response, ifs);
            }
          } else {
            LogSevere("Failed to open: " << path << "\n");
            web->setMessage("Failed to open: " + path);
            web->setError(404);
            sendText = true;
          }
        }

        if (sendText) {
          std::stringstream stream;
          stream << web->getMessage();
          response->write(web->getErrorInternal(), stream);
        }
      } else {
        LogInfo("Sending text\n");
        //    response->write(SimpleWeb::StatusCode::client_error_bad_request, "Testing error");
        //         // FIXME: need to check message was ever set

        auto aSize = web->getMessage().size();

        // Fill in header from web message
        SimpleWeb::CaseInsensitiveMultimap header;
        for (auto& a:web->getHeaderMap()) {
          header.emplace(a.first, a.second);
        }
        header.emplace("Content-Length", to_string(aSize));
        header.emplace("Access-Control-Allow-Origin", "*");
        header.emplace("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        header.emplace("Access-Control-Allow-Headers", "Content-Type, Authorization");
        header.emplace("Access-Control-Expose-Headers", "Content-Length");
        header.emplace("Content-Type", "text/plain; charset=utf-8");

        std::stringstream stream;
        stream << web->getMessage();
        response->write(web->getErrorInternal(), stream, header);
      }
    } else {
      // Algorithm basically reported an error maybe?
    }
  }catch (const std::exception& e) {
    LogSevere("Error handling web request: " << e.what() << "\n");
    // FIXME: do http error codes with library properly
  }
} // WebServer::handleGET

namespace {
// I 'think' this is a call back telling us actual port used for multiconnections
void
serverStarted(unsigned short port)
{
  // WebServer::port=port;
};
};

void
WebServer::runningWebServer()
{
  HttpServer server;

  server.config.port = WebServer::port;

  server.default_resource["GET"] = &WebServer::handleGET;

  server.start(&serverStarted);
}

void
WebServer::startWebServer(const std::string& params, RAPIOProgram * prog)
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

  WebServer::port = port;

  // Create web message queue first
  std::shared_ptr<WebMessageQueue> wmq = std::make_shared<WebMessageQueue>(prog);

  WebMessageQueue::theWebMessageQueue = wmq;

  EventLoop::addEventHandler(wmq);

  // Now add the webserver thread to the event loop
  EventLoop::theThreads.push_back(std::thread(&WebServer::runningWebServer));
  LogInfo("Algorithm Web Server Initialized on port: " << WebServer::port << "\n");
} // WebServer::startWebServer
