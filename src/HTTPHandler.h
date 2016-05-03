#pragma once

#include <memory>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/Router.h"

namespace sakura {

class HTTPHandler : public virtual proxygen::RequestHandler {
 private:
  folly::EventBase* evb_;
  Router* router_;
  std::unique_ptr<proxygen::HTTPMessage> message_;
  std::unique_ptr<folly::IOBuf> body_;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler_;

 public:
  HTTPHandler(
      folly::EventBase* evb,
      Router* router,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler);
  void sendResponse(const HTTPResponse& response);

  /**
   * Invoked when we have successfully fetched headers from client. This will
   * always be the first callback invoked on your handler.
   */
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  /**
   * Invoked when we get part of body for the request.
   */
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

  /**
   * Invoked when the session has been upgraded to a different protocol
   */
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override;

  /**
   * Invoked when we finish receiving the body.
   */
  virtual void onEOM() noexcept override;

  /**
   * Invoked when request processing has been completed and nothing more
   * needs to be done. This may be a good place to log some stats and
   * clean up resources. This is distinct from onEOM() because it is
   * invoked after the response is fully sent. Once this callback has been
   * received, `downstream_` should be considered invalid.
   */
  virtual void requestComplete() noexcept override;

  /**
   * Request failed. Maybe because of read/write error on socket or client
   * not being able to send request in time.
   *
   * NOTE: Can be invoked at any time (except for before onRequest).
   *
   * No more callbacks will be invoked after this. You should clean up after
   * yourself.
   */
  virtual void onError(proxygen::ProxygenError err) noexcept override;

  virtual ~HTTPHandler() noexcept {}
};
}
