#include "src/HTTPResponse.h"

#include <folly/json.h>

using folly::toJson;
using folly::dynamic;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using folly::IOBuf;

namespace sakura {

HTTPResponse::HTTPResponse(int16_t statusCode,
                           const dynamic& body,
                           unordered_map<string, string> headers) {
  response_.setStatusCode(statusCode);
  auto str = toJson(body);
  body_ = IOBuf::copyBuffer(str.data(), str.size());
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse(int16_t statusCode,
                           const string& body,
                           unordered_map<string, string> headers) {
  response_.setStatusCode(statusCode);
  body_ = IOBuf::copyBuffer(body);
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse(int16_t statusCode,
                           unique_ptr<IOBuf> body,
                           unordered_map<string, string> headers) {
  response_.setStatusCode(statusCode);
  body_ = std::move(body);
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse(
    int16_t statusCode,
    const dynamic& body,
    unordered_map<proxygen::HTTPHeaderCode, string> headers) {
  response_.setStatusCode(statusCode);
  auto str = toJson(body);
  body_ = IOBuf::copyBuffer(str.data(), str.size());
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse(
    int16_t statusCode,
    const string& body,
    unordered_map<proxygen::HTTPHeaderCode, string> headers) {
  response_.setStatusCode(statusCode);
  body_ = IOBuf::copyBuffer(body);
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse(
    int16_t statusCode,
    unique_ptr<IOBuf> body,
    unordered_map<proxygen::HTTPHeaderCode, string> headers) {
  response_.setStatusCode(statusCode);
  body_ = std::move(body);
  for (auto& kv : headers) {
    response_.getHeaders().set(kv.first, std::move(kv.second));
  }
}

HTTPResponse::HTTPResponse() {
  response_.setStatusCode(200);
  body_ = IOBuf::create(0);
}

HTTPResponse::HTTPResponse(int16_t statusCode) {
  response_.setStatusCode(statusCode);
  body_ = IOBuf::create(0);
}

HTTPResponse::HTTPResponse(int16_t statusCode, const std::string& body) {
  response_.setStatusCode(statusCode);
  body_ = IOBuf::copyBuffer(body);
}
}
