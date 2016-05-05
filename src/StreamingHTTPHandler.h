#pragma once

#include <memory>

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/Router.h"

namespace sakura {

// template <typename Args...>
// TODO: Use the same onRequest logic that we use in Route
class StreamingHTTPHandler : public virtual proxygen::RequestHandler {
 private:
  folly::EventBase* evb_ = nullptr;
  //folly::Optional<proxygen::ResponseBuilder> responseBuilder_;
  std::unique_ptr<proxygen::ResponseBuilder> responseBuilder_;
  bool sentHeaders_ = false;  // TODO: Maybe need to lock around this
 public:
  StreamingHTTPHandler(){};
  void sendResponse(const HTTPResponse& response);

  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept final;
  // virtual void onRequest(const HTTPRequest& request, Args... args) noexcept =
  // 0;
  virtual void onRequest(const HTTPRequest& request) noexcept = 0;
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override = 0;
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept final;
  virtual void onEOM() noexcept override = 0;
  virtual void requestComplete() noexcept final;
  virtual void onRequestComplete() noexcept = 0;
  virtual void onError(proxygen::ProxygenError err) noexcept final;
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept = 0;
  void sendResponseHeaders(HTTPResponse response) noexcept;
  void sendBody(std::unique_ptr<folly::IOBuf> data) noexcept;
  void sendEOF() noexcept;

  virtual ~StreamingHTTPHandler() noexcept {}
};
}
