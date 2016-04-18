#include "src/HTTPRequest.h"

namespace sakura {
HTTPRequest::HTTPRequest(const proxygen::HTTPMessage& request,
                         std::unique_ptr<folly::IOBuf> body)
    : request_(std::move(request)),
      body_(std::move(body)),
      path_(getUnescapedPath(request_.getPath())),
      queryParams_(HTTPRequest::QueryParams(request_)),
      headers_(HTTPRequest::Headers(request_)),
      cookies_(Cookies(request_)) {}
}
