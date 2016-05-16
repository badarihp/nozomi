#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/regex.hpp>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>
#include <proxygen/httpserver/ResponseHandler.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <wangle/acceptor/TransportInfo.h>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/StreamingHTTPHandler.h"

namespace nozomi {
namespace test {

inline ::testing::AssertionResult assertThrowMsg(
    const char* expr, std::function<::testing::AssertionResult()> f) {
  return f();
}

#define ASSERT_THROW_MSG(BLOCK, EXCEPTION, MESSAGE)                            \
                                                                               \
  ASSERT_PRED_FORMAT1(                                                         \
      nozomi::test::assertThrowMsg, [&]() -> ::testing::AssertionResult {      \
        try {                                                                  \
          BLOCK;                                                               \
          return ::testing::AssertionFailure()                                 \
                 << "did not get an exception of type " << #EXCEPTION;         \
        } catch (const EXCEPTION& e) {                                         \
          std::string msg(e.what());                                           \
          if (msg.find(MESSAGE) == std::string::npos &&                        \
              !boost::regex_match(                                             \
                  e.what(), boost::basic_regex<char>("(\\s?)" MESSAGE))) {     \
            return ::testing::AssertionFailure()                               \
                   << "Message \"" << e.what() << "\""                         \
                   << " did not match expected pattern \"" << MESSAGE << "\""; \
          } else {                                                             \
            return ::testing::AssertionSuccess();                              \
          }                                                                    \
        } catch (const std::exception& e) {                                    \
          return ::testing::AssertionFailure()                                 \
                 << "did not get an exception of type " << #EXCEPTION;         \
        }                                                                      \
      });

template <typename HeaderType = proxygen::HTTPHeaderCode>
HTTPRequest make_request(
    std::string path,
    proxygen::HTTPMethod method = proxygen::HTTPMethod::GET,
    std::unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0),
    std::unordered_map<HeaderType, std::string> headers =
        std::unordered_map<HeaderType, std::string>()) {
  auto message = std::make_unique<proxygen::HTTPMessage>();
  message->setURL(path);
  message->setMethod(method);
  for (const auto& kvp : headers) {
    message->getHeaders().set(kvp.first, kvp.second);
  }
  return HTTPRequest(std::move(message), std::move(body));
}

template <typename... HandlerArgs>
struct TestStreamingHandler : public StreamingHTTPHandler<HandlerArgs...> {
  bool onBodyCalled = false;
  bool onEOMCalled = false;
  bool onRequestReceivedCalled = false;
  bool onRequestCompleteCalled = false;
  bool onUnhandledErrorCalled = false;
  bool setRequestArgsCalled = false;
  proxygen::HTTPMessage request;
  std::tuple<HandlerArgs...> requestArgs;
  std::unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0);

  TestStreamingHandler(folly::EventBase* evb): StreamingHTTPHandler<HandlerArgs...>(evb) {

  }

  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
    onBodyCalled = true;
    body->prependChain(std::move(body));
  }
  virtual void onEOM() noexcept override { onEOMCalled = true; }
  virtual void onRequestReceived(const HTTPRequest& request) noexcept override {
    onRequestReceivedCalled = true;
    this->request = request.getRawRequest();
  }
  virtual void onRequestComplete() noexcept override {
    onRequestCompleteCalled = true;
  }
  virtual void onUnhandledError(proxygen::ProxygenError err) noexcept override {
    onUnhandledErrorCalled = true;
  }
  virtual void setRequestArgs(HandlerArgs... args) override {
    setRequestArgsCalled = true;
    requestArgs =
        std::make_tuple<HandlerArgs...>(std::forward<HandlerArgs>(args)...);
  }
};

struct TestResponseHandler : public proxygen::ResponseHandler {
  wangle::TransportInfo ti;
  int sendChunkHeaderCalls = 0;
  int sendChunkTerminatorCalls = 0;
  int sendEOMCalls = 0;
  int sendAbortCalls = 0;
  int refreshTimeoutCalls = 0;
  int pauseIngressCalls = 0;
  int resumeIngressCalls = 0;
  std::vector<proxygen::HTTPMessage> messages;
  std::vector<std::unique_ptr<folly::IOBuf>> bodies;

  TestResponseHandler(proxygen::RequestHandler* upstream)
      : proxygen::ResponseHandler(upstream) {}

  virtual void sendHeaders(proxygen::HTTPMessage& msg) noexcept {
    messages.push_back(msg);
  }
  virtual void sendChunkHeader(size_t len) noexcept { sendChunkHeaderCalls++; }
  virtual void sendBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    bodies.push_back(std::move(body));
  }
  virtual void sendChunkTerminator() noexcept { sendChunkTerminatorCalls++; }
  virtual void sendEOM() noexcept { sendEOMCalls++; }
  virtual void sendAbort() noexcept { sendAbortCalls++; }
  virtual void refreshTimeout() noexcept { refreshTimeoutCalls++; }
  virtual void pauseIngress() noexcept { pauseIngressCalls++; }
  virtual void resumeIngress() noexcept { resumeIngressCalls++; }
  virtual proxygen::ResponseHandler* newPushedResponse(
      proxygen::PushHandler* pushHandler) noexcept {
    return this;
  };
  // Accessors for Transport/Connection information
  virtual const wangle::TransportInfo& getSetupTransportInfo() const noexcept {
    return ti;
  };
  virtual void getCurrentTransportInfo(wangle::TransportInfo* tinfo) const {}
};
}
}
