#pragma once

#include <chrono>
#include <memory>

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <wangle/concurrent/GlobalExecutor.h>

#include "src/Router.h"

namespace nozomi {

/**
 * Class that waits for the headers and body to arrive before calling
 * a user-defined handler, and sending that response back. Handlers
 * run asynchronously on a threadpool and handlers must return futures
 * in order to be valid.
 */
class HTTPHandler : public virtual proxygen::RequestHandler {
 private:
  std::chrono::milliseconds timeout_;
  Router* router_;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler_;
  std::unique_ptr<folly::IOBuf> body_;
  folly::EventBase* responseEvb_;
  folly::Executor* ioExecutor_;

  std::unique_ptr<proxygen::HTTPMessage> message_;

  folly::Future<folly::Unit> response_;
  folly::Optional<HTTPRequest> request_;

 public:
  /**
   * Creates an HTTPHandler instance
   *
   * @param timeout - How long to wait for the handler to complete. A 503
   *                  is sent if this timeout is exceeded
   * @param router  - The router that was used to create handler. Used
   *                  primarly for fetching error handlers
   * @param handler - The handler to call once the request metadata and body
   *                  have been received.
   * @param responseEvb - The event base that responses should be sent back
   *                      out on. If nullptr, the handler will pull an
   *                      EventBase from the EventBase manager before calling
   *                      handler
   * @param ioExecutor - The executor where handler should be run. If not
   *                     provided, the wangle global IO threadpool is used.
   */
  HTTPHandler(
      std::chrono::milliseconds timeout,
      Router* router,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler,
      folly::EventBase* responseEvb = nullptr,
      folly::Executor* ioExecutor = wangle::getIOExecutor().get());
  virtual ~HTTPHandler() noexcept {}

  /**
   * Sends the response back to the user. This should only be invoked on
   * the correct IO thread, lest the proxygen ResponseBuilder will cause
   * problems.
   */
  void sendResponse(const HTTPResponse& response);

  /**
   * @copydoc proxygen::RequestHandler::onRequest()
   */
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  /**
   * @copydoc proxygen::RequestHandler::ononBody()
   */
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

  /**
   * @copydoc proxygen::RequestHandler::onUpgrade()
   */
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override;

  /**
   * @copydoc proxygen::RequestHandler::onEOM()
   */
  virtual void onEOM() noexcept override;

  /**
   * @copydoc proxygen::RequestHandler::requestComplete()
   */
  virtual void requestComplete() noexcept override;

  /**
   * @copydoc proxygen::RequestHandler::onError()
   */
  virtual void onError(proxygen::ProxygenError err) noexcept override;
};
}
