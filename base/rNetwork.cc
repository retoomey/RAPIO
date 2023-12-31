#include "rCurlConnection.h"
#include "rBoostConnection.h"

#include "rError.h"
#include "rStrings.h"

using namespace rapio;

std::shared_ptr<NetworkConnection> Network::myConnection;

bool
Network::setNetworkEngine(const std::string& key)
{
  try{
    if (key == "CURL") {
      #ifdef HAVE_CURL
      LogInfo("Using CURL as network engine\n");
      myConnection = std::make_shared<CurlConnection>();
      return true;

      #else
      LogInfo("CURL not available...\n");
      #endif
    }
    LogInfo("Using BOOST::asio as network engine\n");
    myConnection = std::make_shared<BoostConnection>();
  }catch (const std::runtime_error& e) {
    LogSevere(e.what() << "\n");
    myConnection = nullptr; // gonna crash later
    return false;
  }
  return true;
}
