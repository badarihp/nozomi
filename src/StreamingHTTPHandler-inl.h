#pragma once

#include <folly/io/async/EventBaseManager.h>

namespace nozomi {

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::onRequest(
    std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
  DCHECK(downstream_ != nullptr);
  if (evb_ == nullptr) {
    evb_ = folly::EventBaseManager::get()->getEventBase();
  }
  responseBuilder_ = std::make_unique<proxygen::ResponseBuilder>(downstream_);
  onRequestReceived(HTTPRequest(std::move(headers), folly::IOBuf::create(0)));
}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::onUpgrade(
    proxygen::UpgradeProtocol) noexcept {}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::requestComplete() noexcept {
  // TODO: Ensure that we've sent the responses before deleting, this /should/
  // be okay, since requestComplete doesn't get called until the EOF has been
  // sent
  onRequestComplete();
  evb_->runInEventBaseThread([this]() { delete this; });
}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::onError(
    proxygen::ProxygenError err) noexcept {
  // TODO: Ensure that we've sent the responses before deleting, this /should/
  // be okay, since requestComplete doesn't get called until the EOF has been
  // sent
  onUnhandledError(err);
  evb_->runInEventBaseThread([this]() { delete this; });
}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::sendResponseHeaders(
    HTTPResponse response) noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_ == false);
  sentHeaders_ = true;
  evb_->runInEventBaseThread(
      [ response = std::move(response), this ]() mutable {
        responseBuilder_->status(response.getStatusCode(), "");
        response.getHeaders().getHeaders().forEach(
            [this](const auto& header, const auto& value) {
              responseBuilder_->header(header, value);
            });
        auto body = response.getBody();
        // ResponseBuilder API doesn't allow 0 length bodies
        if (body->length() != 0) {
          responseBuilder_->body(response.getBody());
        }
        responseBuilder_->send();
      });
}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::sendBody(
    std::unique_ptr<folly::IOBuf> data) noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_);
  // ResponseBuilder API doesn't allow 0 length bodies
  if (data->length() == 0) {
    return;
  }
  evb_->runInEventBaseThread([ data = std::move(data), this ]() mutable {
    responseBuilder_->body(std::move(data)).send();
  });
}

template <typename... HandlerArgs>
void StreamingHTTPHandler<HandlerArgs...>::sendEOF() noexcept {
  DCHECK(evb_ != nullptr);
  DCHECK(sentHeaders_);
  evb_->runInEventBaseThread(
      [this]() mutable { responseBuilder_->sendWithEOM(); });
}
}
