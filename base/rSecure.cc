#include "rSecure.h"

#include "rError.h"

#include <iostream>
#include <iomanip>


#if HAVE_OPENSSL
# include <openssl/err.h>
# include <openssl/ssl.h>
# include <openssl/hmac.h>
# include <openssl/sha.h>
# include <openssl/conf.h>
# include <openssl/evp.h>
#endif

using namespace rapio;

std::string
Secure::sign(const std::string& key, const std::string& message)
{
  std::string out;

  #if HAVE_OPENSSL

  # ifdef EVP_MAX_MD_SIZE
  unsigned int result_len = -1;
  unsigned char result[EVP_MAX_MD_SIZE];
  // FIXME: Seems to be handling utf8 conversion...python did this
  // explicitly.  How to check?
  HMAC(EVP_sha256(),
    key.c_str(), key.size(),
    (const unsigned char *) message.c_str(), message.size(),
    result, &result_len);

  //  out = std::string(result);

  for (size_t i = 0; i < result_len; i++) {
    out += result[i];
  }
  # else // ifdef EVP_MAX_MD_SIZE
  // FIXME: Complain?
  # endif // ifdef EVP_MAX_MD_SIZE
  #else // if HAVE_OPENSSL
  fLogSevere("Compiled without openssl support.");
  #endif // if HAVE_OPENSSL
  return out;
}

std::string
Secure::hexdigest(const std::string& bytes)
{
  std::stringstream ss;

  for (size_t i = 0; i < bytes.length(); i++) {
    // Clip to unsigned char, this is why should be utf-8
    unsigned char x = bytes[i] & 0xFF;
    // ss << std::hex << std::setw(2) << std::setfill('0') << (unsigned char)bytes[i];
    ss << std::hex << std::setw(2) << std::setfill('0') << (int) x;
  }
  return ss.str();
}

std::vector<uint8_t>
Secure::decode64(const std::string& input)
{
  std::vector<uint8_t> outbuf;

  #if HAVE_OPENSSL
  BIO * b64, * bmem;
  char inbuf[512];
  int inlen;

  b64 = BIO_new(BIO_f_base64());

  bmem = BIO_new_mem_buf(input.c_str(), input.size());
  bmem = BIO_push(b64, bmem);

  while ((inlen = BIO_read(b64, inbuf, 512)) > 0) {
    size_t len = (inlen < 0) ? 0 : (size_t) (inlen);
    for (size_t i = 0; i < len; i++) {
      outbuf.push_back(inbuf[i]); // ok to downcast we want it to break
    }
  }
  BIO_free_all(b64); // this frees bmem too
  #else // if HAVE_OPENSSL
  fLogSevere("Compiled without openssl support.");
  #endif // if HAVE_OPENSSL
  return outbuf;
}

std::string
Secure::sha256(const std::string& str)
{
  std::stringstream ss;

  #if HAVE_OPENSSL
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;

  // Ignore SHA256_Init deprecated for now
  # pragma GCC diagnostic push
  # pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, str.c_str(), str.size());
  SHA256_Final(hash, &sha256);
  # pragma GCC diagnostic pop

  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    // ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    ss << hash[i];
  }
  #else // if HAVE_OPENSSL
  fLogSevere("Compiled without openssl support.");
  #endif // if HAVE_OPENSSL
  return ss.str();
}

namespace {
// Callback for openssl error messages
int
opensslerror(const char * str, size_t len, void * u)
{
  fLogSevere("OpenSSL error: {}", str);
  /** FIXME: Documentation can't tell what we're supposed to return here */
  return 0;
}
}

bool
Secure::validateSigned(
  const std::string& publickey,
  const std::string& message,
  const std::string& signature,
  bool             base64) // convert from base64 or not (binary already)
{
  #if HAVE_OPENSSL
  const char * hold = publickey.c_str();

  // Memory stuff
  EVP_MD_CTX * mdctx = NULL;
  BIO * bufio        = NULL;

  bool error = true;
  bool pass  = false;

  while (1) { // break on error/success
    bufio = BIO_new_mem_buf((void *) hold, publickey.size());
    if (bufio == NULL) { break; }
    // Don't free this one (will cause double free)
    EVP_PKEY * evp_key = PEM_read_bio_PUBKEY(bufio, NULL, NULL, NULL);
    if (evp_key == NULL) {
      fLogSevere("Invalid public key for signature checking");
      break;
    }

    /*  Some testing stuff for validating RSA
     *     rsa_pubkey = EVP_PKEY_get1_RSA(evp_key);
     *
     *     evp_verify_key = EVP_PKEY_new();
     *     ret = EVP_PKEY_assign_RSA(evp_verify_key, rsa_pubkey);
     *     if (ret != 1){
     *       break;
     *     }
     */

    // Check signature context
    mdctx = EVP_MD_CTX_create();

    const EVP_MD * md = EVP_sha256();
    int ret = EVP_DigestVerifyInit(mdctx, NULL, md, NULL, evp_key);
    if (ret != 1) {
      break;
    }

    // Add all xml message to be verified...
    size_t msglen = message.size();
    ret = EVP_DigestVerifyUpdate(mdctx, message.c_str(), msglen);
    if (ret != 1) {
      break;
    }

    // Signature as either base64 or binary
    std::vector<uint8_t> outvec;
    if (base64) {
      outvec = decode64(signature);
    } else {
      for (size_t i = 0; i < signature.size(); i++) {
        outvec.push_back(signature[i]);
      }
    }
    uint8_t * ubuf  = (uint8_t *) (&(outvec[0]));
    size_t ubufsize = outvec.size();

    ret = EVP_DigestVerifyFinal(mdctx, ubuf, ubufsize);
    if (ret != 1) {
      fLogSevere("Signature mismatch on passed data.");
      break;
    } else {
      fLogInfo("Signature match on passed data.");
      pass  = true;
      error = false;
    }
    break;
  }

  // Any error condition print out
  if (error) {
    ERR_print_errors_cb(opensslerror, 0);
  }

  // Memory clean up
  EVP_MD_CTX_destroy(mdctx);
  BIO_free(bufio);

  return pass;

  #else // if HAVE_OPENSSL
  fLogSevere("Compiled without openssl support.");
  return false;

  #endif // if HAVE_OPENSSL
} // Secure::validateSigned
