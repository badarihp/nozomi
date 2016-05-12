#pragma once

#include <folly/io/IOBuf.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StreamingHTTPHandler.h"

namespace sakura {
class StreamingFileHandler : public StreamingHTTPHandler<int64_t> {
 public:
  virtual ~StreamingFileHandler() {}
  virtual void onRequest(const HTTPRequest& request) noexcept override;
  virtual void setRequestArgs(int64_t) noexcept override;
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  virtual void onEOM() noexcept override;
  virtual void onRequestComplete() noexcept override;
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept override;
};
}