#pragma once

#include <memory>

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

namespace nozomi {

template <typename... HandlerArgs>
// TODO: Use the same onRequest logic that we use in Route
class StreamingHTTPHandler : public virtual proxygen::RequestHandler {
 private:
  folly::EventBase* evb_ = nullptr;
  // folly::Optional<proxygen::ResponseBuilder> responseBuilder_;
  std::unique_ptr<proxygen::ResponseBuilder> responseBuilder_;
  bool sentHeaders_ = false;  // TODO: Maybe need to lock around this
 public:
  StreamingHTTPHandler(){};
  virtual ~StreamingHTTPHandler() noexcept {}

  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept = 0;
  virtual void onEOM() noexcept = 0;
  virtual void onRequest(const HTTPRequest& request) noexcept = 0;
  virtual void onRequestComplete() noexcept = 0;
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept = 0;
  virtual void setRequestArgs(HandlerArgs... args) = 0;
  virtual void onError(proxygen::ProxygenError err) noexcept final;
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept final;
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept final;
  virtual void requestComplete() noexcept final;
  void sendResponseHeaders(HTTPResponse response) noexcept;
  void sendBody(std::unique_ptr<folly::IOBuf> data) noexcept;
  void sendEOF() noexcept;
};
}

#include "src/StreamingHTTPHandler-inl.h"
