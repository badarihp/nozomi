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

/**
 * Creates a handler for streaming HTTP data. This is more useful
 * for larger requests that may not all fit in memory at once
 *
 * @tparam HandlerArgs Arguments parsed out from the HTTP request per
 *                     the logic in the Route that created this handler
 */
template <typename... HandlerArgs>
class StreamingHTTPHandler : public proxygen::RequestHandler {
 private:
  folly::EventBase* evb_ = nullptr;
  std::unique_ptr<proxygen::ResponseBuilder> responseBuilder_;
  bool sentHeaders_ = false;  // TODO: Maybe need to lock around this

 public:
  StreamingHTTPHandler(folly::EventBase* evb = nullptr): evb_(evb){};
  virtual ~StreamingHTTPHandler() noexcept {}

  /**
   * Called when a piece of the request body arrives. This may be
   * called multiple times
   */
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept = 0;

  /**
   * @copydoc proxygen::RequestHandler::onEOM()
   */
  virtual void onEOM() noexcept = 0;

  /**
   * Called once the HTTP headers and metadat have been sent
   * by the client
   */
  virtual void onRequestReceived(const HTTPRequest& request) noexcept = 0;

  /**
   * Called once the response has been sent
   */
  virtual void onRequestComplete() noexcept = 0;

  /**
   * Called when there is an error in client communication
   */
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept = 0;

  /**
   * Called with arguments extracted from the HTTP request in the route parser
   */
  virtual void setRequestArgs(HandlerArgs... args) = 0;
 
  /**
   * Should be called by the implementation class when response headers are ready
   * This needs to be called before sendBody()
   */
  void sendResponseHeaders(HTTPResponse response) noexcept;

  /**
   * Should be called by the implementation class when data should be sent
   * to the client. This must be called after sendResponseHeader()
   */
  void sendBody(std::unique_ptr<folly::IOBuf> data) noexcept;

  /**
   * Should be called by the implementation class when all data has
   * been sent to the client
   */
  void sendEOF() noexcept;

  virtual void onError(proxygen::ProxygenError err) noexcept final;
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept final;
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept final;
  virtual void requestComplete() noexcept final;
};
}

#include "src/StreamingHTTPHandler-inl.h"
