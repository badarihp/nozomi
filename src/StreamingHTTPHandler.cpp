#include "src/StreamingHTTPHandler.h"

#include <folly/io/async/EventBaseManager.h>

namespace sakura {

void StreamingHTTPHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept  {
  DCHECK(downstream_ != nullptr);
  evb_ = folly::EventBaseManager::get()->getEventBase();
  responseBuilder_ = std::make_unique<proxygen::ResponseBuilder>(downstream_);
  onRequest(HTTPRequest(std::move(headers), folly::IOBuf::create(0)));
}

void StreamingHTTPHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept  {}

void StreamingHTTPHandler::requestComplete() noexcept  {
  onRequestComplete();
  delete this;
}

void StreamingHTTPHandler::onError(proxygen::ProxygenError err) noexcept  {
  onUnhandledError(err);
  delete this;
}

void StreamingHTTPHandler::sendResponseHeaders(HTTPResponse response) noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_ == false);
  sentHeaders_ = true;
  evb_->runInEventBaseThread([ response = std::move(response), this ]() mutable {
    responseBuilder_->status(response.getStatusCode(), "");
    response.getHeaders().getHeaders().forEach(
        [this](const auto& header, const auto& value) {
          responseBuilder_->header(header, value);
        });
    responseBuilder_->body(response.getBody()).send();
  });
}

void StreamingHTTPHandler::sendBody(std::unique_ptr<folly::IOBuf> data) noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_);
  evb_->runInEventBaseThread([ data = std::move(data), this ]() mutable {
    responseBuilder_->body(std::move(data)).send();
  });
}

void StreamingHTTPHandler::sendEOF() noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_);
  responseBuilder_->sendWithEOM();
}
}
