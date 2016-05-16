#include "src/HTTPRequest.h"

#include <glog/logging.h>

namespace nozomi {
HTTPRequest::HTTPRequest(std::unique_ptr<proxygen::HTTPMessage> request,
                         std::unique_ptr<folly::IOBuf> body)
    : request_(std::move(request)),
      body_(std::move(body)),
      queryParams_(HTTPRequest::QueryParams(request_.get())),
      headers_(HTTPRequest::Headers(request_.get())),
      cookies_(Cookies(request_.get())) {
  DCHECK(request_ != nullptr);
  DCHECK(body_ != nullptr);
  auto methodAndPath = getMethodAndPath(request_.get());
  method_ = std::get<0>(methodAndPath);
  path_ = std::move(std::get<1>(methodAndPath));
}

std::tuple<proxygen::HTTPMethod, std::string> HTTPRequest::getMethodAndPath(
    const proxygen::HTTPMessage* message) {
  DCHECK(message != nullptr);
  return std::make_tuple<proxygen::HTTPMethod, std::string>(
      message->getMethod().value_or(proxygen::HTTPMethod::GET),
      getUnescapedPath(message->getPath()));
}
}
