#include "src/HTTPHandler.h"

#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <wangle/concurrent/GlobalExecutor.h>

#include <glog/logging.h>

using folly::EventBase;
using folly::IOBuf;
using std::shared_ptr;
using std::unique_ptr;
using proxygen::HTTPMessage;
using proxygen::ProxygenError;
using proxygen::ResponseBuilder;

// TODO: Tests need to be updated, and this needs to be injectable
namespace nozomi {
HTTPHandler::HTTPHandler(
    Router* router,
    std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler,
    EventBase* evb)
    : evb_(evb),
      router_(router),
      handler_(std::move(handler)),
      body_(IOBuf::create(0)) {
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

  builder.body(response.getBody()).sendWithEOM();  // For streaming calls, we'll
                                                   // use chunked encoding in a
                                                   // different handler
}

void HTTPHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  DCHECK(message_ == nullptr);
  message_ = std::move(headers);
};

/**
 * Invoked when we get part of body for the request.
 */
void HTTPHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  body_->prependChain(std::move(body));
};

/**
 * Invoked when the session has been upgraded to a different protocol
 */
void HTTPHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {
    // TODO: Websockets
};

/**
 * Invoked when we finish receiving the body.
 */
void HTTPHandler::onEOM() noexcept {
  // TODO: Future timeout
  request_ = HTTPRequest(std::move(message_), std::move(body_));

  /*
   * We have to pull in the evb from the EventBaseManager because the evb
   * that we're handed in the factory is not the one that we need to use to
   * make responses. Also, when we make responses, the response builder /
   * response handler calls "runInLoop", which will break if we're running
   * in the wrong thread.
   */
  auto* evb = evb_ != nullptr ? evb_ : folly::EventBaseManager::get()->getEventBase();
  // TODO: Tests
  try {
    response_ = via(wangle::getIOExecutor().get(),
                    [this]() { return handler_(*request_); })
                    .onError([this](const std::exception& e) {
                      return router_->getErrorHandler(500)(*request_).onError(
                          [](const std::exception& e) {
                            return HTTPResponse(500, "Unknown error");
                          });
                    })
                    .onTimeout(std::chrono::seconds(1), [this](){ return HTTPResponse(503); })
                    .then(evb, [this, evb](const HTTPResponse& response) {
                      sendResponse(response);
                    });
  } catch (const std::exception& e) {
    response_ = via(
        evb, [this]() { sendResponse(HTTPResponse(500, "Unknown error")); });
  }
};

/**
 * Invoked when request processing has been completed and nothing more
 * needs to be done. This may be a good place to log some stats and
 * clean up resources. This is distinct from onEOM() because it is
 * invoked after the response is fully sent. Once this callback has been
 * received, `downstream_` should be considered invalid.
 */
void HTTPHandler::requestComplete() noexcept {
  // This is not called until after the response is sent,
  // so deleting here is fine. This is the pattern that's
  // used in all of the proxygen example code
  delete this;
};

/**
 * Request failed. Maybe because of read/write error on socket or client
 * not being able to send request in time.
 *
 * NOTE: Can be invoked at any time (except for before onRequest).
 *
 * No more callbacks will be invoked after this. You should clean up after
 * yourself.
 */
void HTTPHandler::onError(ProxygenError err) noexcept { delete this; };
}
