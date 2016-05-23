#include "src/HTTPHandler.h"

#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <glog/logging.h>

using folly::EventBase;
using folly::EventBaseManager;
using folly::Executor;
using folly::Future;
using folly::IOBuf;
using std::shared_ptr;
using std::unique_ptr;
using proxygen::HTTPMessage;
using proxygen::ProxygenError;
using proxygen::ResponseBuilder;

namespace nozomi {

HTTPHandler::HTTPHandler(
    std::chrono::milliseconds timeout,
    Router* router,
    std::function<Future<HTTPResponse>(const HTTPRequest&)> handler,
    EventBase* responseEvb,
    Executor* ioExecutor)
    : timeout_(timeout),
      router_(router),
      handler_(std::move(handler)),
      body_(IOBuf::create(0)),
      responseEvb_(responseEvb),
      ioExecutor_(ioExecutor) {
  DCHECK(router != nullptr);
}

void HTTPHandler::sendResponse(const HTTPResponse& response) {
  // TODO: Add headers to response if missing
  // Yes, this copies the headers object, but I'd rather have a clean
  // response builder ATM
  auto builder = ResponseBuilder(downstream_);
  builder.status(response.getStatusCode(), "");
  response.getHeaders().getHeaders().forEach(
      [&builder](const auto& header, const auto& value) {
        builder.header(header, value);
      });

  builder.body(response.getBody()).sendWithEOM();
}

void HTTPHandler::onRequest(unique_ptr<HTTPMessage> headers) noexcept {
  DCHECK(message_ == nullptr);
  message_ = std::move(headers);
};

void HTTPHandler::onBody(unique_ptr<IOBuf> body) noexcept {
  body_->prependChain(std::move(body));
};

void HTTPHandler::onUpgrade(proxygen::UpgradeProtocol ) noexcept {
    // TODO: Websockets
};

void HTTPHandler::onEOM() noexcept {
  request_ = HTTPRequest(std::move(message_), std::move(body_));

  /*
   * We have to pull in the evb from the EventBaseManager because the evb
   * that we're handed in the factory is not the one that we need to use to
   * make responses. Also, when we make responses, the response builder /
   * response handler calls "runInLoop", which will break if we're running
   * in the wrong thread.
   */
  auto* evb = responseEvb_ != nullptr ? responseEvb_
                                      : EventBaseManager::get()->getEventBase();
  response_ = via(ioExecutor_, [this]() { return handler_(*request_); })
                  .onError([this](const std::exception& e) {
                    return router_->getErrorHandler(500)(*request_);
                  })
                  .onTimeout(timeout_,
                             [this]() {
                               return router_->getErrorHandler(503)(*request_);
                             })
                  .onError([](const std::exception& e) {
                    return HTTPResponse(500, "Unknown error");
                  })
                  .then(evb, [this, evb](const HTTPResponse& response) {
                    sendResponse(response);
                  });
};

void HTTPHandler::requestComplete() noexcept {
  // This is not called until after the response is sent,
  // so deleting here is fine. This is the pattern that's
  // used in all of the proxygen example code
  delete this;
};

void HTTPHandler::onError(ProxygenError ) noexcept {
  // Once this is called, no other callbacks will be run, so it's safe to
  // delete here. This is the pattern used in proxygen example code
  delete this;
};
}
