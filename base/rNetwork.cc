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
      fLogInfo("Using CURL as network engine");
      myConnection = std::make_shared<CurlConnection>();
      return true;

      #else
      fLogInfo("CURL not available...");
      #endif
    }
    fLogInfo("Using BOOST::asio as network engine");
    myConnection = std::make_shared<BoostConnection>();
  }catch (const std::runtime_error& e) {
    fLogSevere("{}", e.what());
    myConnection = nullptr; // gonna crash later
    return false;
  }
  return true;
}
