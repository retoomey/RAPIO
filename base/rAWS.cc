#include "rAWS.h"
#include "rSecure.h"

#include "rError.h"
#include "rTime.h"
#include "rNetwork.h"

#include <iostream>
#include <iomanip>

using namespace rapio;

std::string
AWS::getSignatureKey(
  const std::string& key,
  const std::string& dateStamp,
  const std::string& regionName,
  const std::string& serviceName)
{
  // FIXME: utf8 enforce?
  const std::string kDate    = Secure::sign("AWS4" + key, dateStamp);
  const std::string kRegion  = Secure::sign(kDate, regionName);
  const std::string kService = Secure::sign(kRegion, serviceName);
  const std::string kSigning = Secure::sign(kService, "aws4_request");

  return kSigning;
}

void
AWS::test_REST_API_Get(
  const std::string& access_key,
  const std::string& secret_key)
{
  fLogInfo("Ok here in get REST test");

  std::string method   = "GET";
  std::string service  = "ec2";
  std::string host     = "ec2.amazonaws.com";
  std::string region   = "us-east-1";
  std::string endpoint = "https://ec2.amazonaws.com";
  std::string request_parameters = "Action=DescribeRegions&Version=2013-10-15";

  // for(size_t i=0; i< 100000; i++){

  // Create a data for headers and the credential string
  rapio::Time aTime(Time::CurrentTime());
  const std::string amzdate = aTime.getString("%Y%m%dT%H%M%SZ");
  // const std::string amzdate = aTime.getString("%Y%m%dT%H%MZ"); For match testing with python
  const std::string datestamp = aTime.getString("%Y%m%d");

  // ************* TASK 1: CREATE A CANONICAL REQUEST *************
  // http://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html

  // Step 1 is to define the verb (GET, POST, etc.)--already done.

  // Step 2: Create canonical URI--the part of the URI from domain to query
  // string (use '/' if no path)
  std::string canonical_uri = "/";

  // Step 3: Create the canonical query string. In this example (a GET request),
  // request parameters are in the query string. Query string values must
  // be URL-encoded (space=%20). The parameters must be sorted by name.
  // For this example, the query string is pre-formatted in the request_parameters variable.
  std::string canonical_querystring = request_parameters;

  // Step 4: Create the canonical headers and signed headers. Header names
  // must be trimmed and lowercase, and sorted in code point order from
  // low to high. Note that there is a trailing \n.
  std::string canonical_headers = "host:" + host + "\n" + "x-amz-date:" + amzdate + "\n";

  // Step 5: Create the list of signed headers. This lists the headers
  // in the canonical_headers list, delimited with ";" and in alpha order.
  // Note: The request can include any headers; canonical_headers and
  // signed_headers lists those that you want to be included in the
  // hash of the request. "Host" and "x-amz-date" are always required.
  std::string signed_headers = "host;x-amz-date";

  // Step 6: Create payload hash (hash of the request body content). For GET
  // requests, the payload is an empty string ("").

  // std::string payload_hash = hashlib.sha256(('').encode('utf-8')).hexdigest()
  const std::string payload_hash = Secure::hexdigest(Secure::sha256("")); // guessing here

  fLogInfo("Payload_hash: {}", payload_hash);

  // Step 7: Combine elements to create canonical request
  std::string canonical_request = method + "\n" + canonical_uri + "\n" + canonical_querystring + "\n"
    + canonical_headers + "\n" + signed_headers + "\n" + payload_hash;

  // ************* TASK 2: CREATE THE STRING TO SIGN*************
  // Match the algorithm to the hashing algorithm you use, either SHA-1 or
  // SHA-256 (recommended)
  std::string algorithm        = "AWS4-HMAC-SHA256";
  std::string credential_scope = datestamp + "/" + region + "/" + service + "/" + "aws4_request";

  fLogInfo("SCOPE:{}", credential_scope);
  fLogInfo("ALGORITHM:{}", algorithm);
  fLogInfo("AMZDATE:{}", amzdate);
  std::string string_to_sign = algorithm + "\n" + amzdate + "\n"
    + credential_scope + "\n" + Secure::hexdigest(Secure::sha256(canonical_request));
  std::string testcan = Secure::hexdigest(Secure::sha256(canonical_request));

  fLogInfo("CAN:{}", testcan);

  // ************* TASK 3: CALCULATE THE SIGNATURE *************
  // Create the signing key using the function defined above.

  // std::string payload_hash = hashlib.sha256(('').encode('utf-8')).hexdigest()
  // const std::string payload_hash = sha256(""); // guessing here

  //   const std::string testing = aTime.getString("%8Y%m%d-%H%M%S.%/ms", true);
  // fLogInfo("Got this thing: {} {}", amzdate, datestamp);
  //   fLogInfo("Got this thing: {}", testing);
  // }
  std::string signing_key = getSignatureKey(secret_key, datestamp, region, service);

  for (size_t i = 0; i < signing_key.length(); i++) {
    std::cout << (int) (signing_key[i]) << ",";
  }
  std::cout << "\n";

  // Sign the string_to_sign using the signing_key
  // signature = hmac.new(signing_key, (string_to_sign).encode('utf-8'), hashlib.sha256).hexdigest()
  std::string signature = Secure::hexdigest(Secure::sign(signing_key, string_to_sign));

  fLogInfo("SIGN:{}", signature);

  // ************* TASK 4: ADD SIGNING INFORMATION TO THE REQUEST *************
  // The signing information can be either in a query string value or in
  // a header named Authorization. This code shows how to use a header.
  // Create authorization header and add to request headers
  std::string authorization_header = algorithm + " " + "Credential=" + access_key + "/" + credential_scope + ", "
    + "SignedHeaders=" + signed_headers + ", " + "Signature=" + signature;

  fLogInfo("AUTH:{}", authorization_header);

  std::vector<std::string> headers;

  headers.push_back("x-amz-date: " + amzdate);
  headers.push_back("Authorization: " + authorization_header);

  std::string request_url = endpoint + '?' + canonical_querystring;

  std::vector<char> buf;
  int ret = Network::readH(request_url, headers, buf);

  if (ret < 0) {
    fLogInfo("Failed looks like");
  } else {
    for (auto c:buf) {
      std::cout << c;
    }
    std::cout << "\n";
  }

  // ramp::CurlConnection::read1(
  // headers = {'x-amz-date':amzdate, 'Authorization':authorization_header}

  /*
   * BYTE digest[20];
   * CHMAC_SHA1 hmac_sha1;
   * hmac_sha1.HMAC_SHA1((BYTE*)request.c_str(), request.size(),(BYTE*)m_privateKey.c_str(),m_privateKey.size(),digest);
   * std::string signature(base64_encode((unsigned char*)digest,20));
   */

  /*std::string signature = "*";
   * for(size_t i=0; i<20; i++){
   * signature += digest[i];
   * }
   */
  //  signature += "*";
  //  fLogInfo("{}",signature);
  //  */
  // fLogInfo("{}", fred);
} // AWS::test_REST_API_Get
