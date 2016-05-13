#include <memory>
#include <string>
#include <unordered_map>

#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"

namespace sakura {
namespace test {

template <typename HeaderType = proxygen::HTTPHeaderCode>
HTTPRequest make_request(
    std::string path,
    proxygen::HTTPMethod method = proxygen::HTTPMethod::GET,
    std::unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0),
    std::unordered_map<HeaderType, std::string> headers =
        std::unordered_map<HeaderType, std::string>()) {
  auto message = std::make_unique<proxygen::HTTPMessage>();
  message->setURL(path);
  message->setMethod(method);
  for (const auto& kvp : headers) {
    message->getHeaders().set(kvp.first, kvp.second);
  }
  return HTTPRequest(std::move(message), std::move(body));
}
}
}
