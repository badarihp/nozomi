#include "src/HTTPHandler.h"

#include <proxygen/httpserver/ResponseBuilder.h>

#include <glog/logging.h>

using folly::IOBuf;
using std::shared_ptr;
using std::unique_ptr;
using proxygen::HTTPMessage;
using proxygen::ProxygenError;
using proxygen::ResponseBuilder;

namespace sakura {
HTTPHandler::HTTPHandler(
    Router* router,
    std::function<HTTPResponse(const HTTPRequest&)> handler)
    : router_(router), handler_(std::move(handler)), body_(IOBuf::create(0)) { DCHECK(router != nullptr);
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
  // TODO: This request probably needs to be a shared_ptr instead
  //      when I get to the point that I have futures. Async gets
  //      tricky with just references
  auto request = HTTPRequest(std::move(message_), std::move(body_));
  try {
    sendResponse(handler_(std::move(request)));
  } catch (const std::exception& e) {
    try {
      LOG(INFO) << "Error: " << e.what();
      sendResponse(router_->getErrorHandler(500)(request));
    } catch (const std::exception& e) {
      sendResponse(HTTPResponse(500, "Unknown error"));
    }
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
  //TODO: When moved to futures, the delete needs to be conditional
  //      on whether the response has been sent
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
void HTTPHandler::onError(ProxygenError err) noexcept {
  //TODO: When moved to futures, the delete needs to be conditional
  //      on whether the response has been sent
  delete this;
};
}
