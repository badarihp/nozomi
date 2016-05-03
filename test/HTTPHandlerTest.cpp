#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/ResponseHandler.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <wangle/acceptor/TransportInfo.h>

#include "src/HTTPHandler.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/StaticRoute.h"
#include "src/StringUtils.h"

using folly::IOBuf;

using namespace std;
using namespace proxygen;

namespace sakura {
namespace test {

struct CustomHandler : public virtual proxygen::ResponseHandler {
  wangle::TransportInfo ti;
  int sendChunkHeaderCalls = 0;
  int sendChunkTerminatorCalls = 0;
  int sendEOMCalls = 0;
  int sendAbortCalls = 0;
  int refreshTimeoutCalls = 0;
  int pauseIngressCalls = 0;
  int resumeIngressCalls = 0;
  vector<HTTPMessage> messages;
  vector<std::unique_ptr<IOBuf>> bodies;

  CustomHandler(proxygen::RequestHandler* upstream)
      : proxygen::ResponseHandler(upstream) {}

  virtual void sendHeaders(HTTPMessage& msg) noexcept {
    messages.push_back(msg);
  }
  virtual void sendChunkHeader(size_t len) noexcept { sendChunkHeaderCalls++; }
  virtual void sendBody(unique_ptr<IOBuf> body) noexcept {
    bodies.push_back(std::move(body));
  }
  virtual void sendChunkTerminator() noexcept { sendChunkTerminatorCalls++; }
  virtual void sendEOM() noexcept { sendEOMCalls++; }
  virtual void sendAbort() noexcept { sendAbortCalls++; }
  virtual void refreshTimeout() noexcept { refreshTimeoutCalls++; }
  virtual void pauseIngress() noexcept { pauseIngressCalls++; }
  virtual void resumeIngress() noexcept { resumeIngressCalls++; }
  virtual ResponseHandler* newPushedResponse(
      PushHandler* pushHandler) noexcept {
    return this;
  };
  // Accessors for Transport/Connection information
  virtual const wangle::TransportInfo& getSetupTransportInfo() const noexcept {
    return ti;
  };
  virtual void getCurrentTransportInfo(wangle::TransportInfo* tinfo) const {}
};

struct HTTPHandlerTest : public ::testing::Test {
  function<folly::Future<HTTPResponse>(const HTTPRequest&)> errorHandler;
  function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;

  std::unique_ptr<HTTPMessage> requestMessage;
  std::unique_ptr<IOBuf> body;
  folly::EventBase evb;
  Router router;
  HTTPHandler httpHandler;
  CustomHandler responseHandler;

  HTTPHandlerTest()
      : requestMessage(std::make_unique<HTTPMessage>()),
        body(IOBuf::copyBuffer("the body")),
        router(make_router(
            {{500, function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                       [this](const HTTPRequest& request) {
                         return errorHandler(request);
                       })}},
            make_route(
                "/",
                {proxygen::HTTPMethod::GET},
                function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                    [this](const HTTPRequest& request) {
                      return handler(request);
                    })))),
        httpHandler(
            &evb,
            &router,
            [this](const HTTPRequest& request) { return handler(request); }),
        responseHandler(&httpHandler) {
    requestMessage->getHeaders().set("Content-Type", "text/plain");
    requestMessage->setMethod(proxygen::HTTPMethod::GET);
    requestMessage->setURL("/");
    httpHandler.setResponseHandler(&responseHandler);
  }
};

TEST_F(HTTPHandlerTest, sends_response) {
  std::string capturedHeader;
  std::string capturedBody;

  errorHandler = [](const HTTPRequest& request) {
    return HTTPResponse::future(504);
  };
  handler = [&](const HTTPRequest& request) {
    capturedHeader = request.getHeaders()["Content-Type"].value();
    capturedBody = request.getBodyAsString();
    return HTTPResponse::fromString(201, "Body goes here",
                                    {{"Location", "http://example.com"}});
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onBody(body->clone());
  httpHandler.onEOM();

  ASSERT_EQ("text/plain", capturedHeader);
  ASSERT_EQ("the body", capturedBody);
  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(201, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(
      "http://example.com",
      responseHandler.messages[0].getHeaders().getSingleOrEmpty("Location"));
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Body goes here", to_string(responseHandler.bodies[0]));
}

TEST_F(HTTPHandlerTest, sends_500_on_uncaught_exception_in_handler) {
  errorHandler = [&](const HTTPRequest& request) {
    return HTTPResponse::fromString(504, "Body goes here",
                                    {{"Location", "http://example.com"}});
  };
  handler = [](const HTTPRequest& request) -> HTTPResponse {
    throw std::runtime_error("Broken");
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onBody(body->clone());
  httpHandler.onEOM();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(504, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(
      "http://example.com",
      responseHandler.messages[0].getHeaders().getSingleOrEmpty("Location"));
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Body goes here", to_string(responseHandler.bodies[0]));
}

TEST_F(HTTPHandlerTest, sends_generic_500_when_custom_500_handler_throws) {
  errorHandler = [&](const HTTPRequest& request) -> HTTPResponse {
    throw std::runtime_error("Broken");
  };
  handler = [](const HTTPRequest& request) -> HTTPResponse {
    throw std::runtime_error("Broken");
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onBody(body->clone());
  httpHandler.onEOM();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(500, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Unknown error", to_string(responseHandler.bodies[0]));
}

TEST_F(HTTPHandlerTest, sends_empty_buffer_when_no_body_sent) {
  std::string capturedHeader;
  std::string capturedBody;

  errorHandler = [](const HTTPRequest& request) {
    return HTTPResponse::future(504);
  };
  handler = [&](const HTTPRequest& request) {
    capturedHeader = request.getHeaders()["Content-Type"].value();
    capturedBody = request.getBodyAsString();
    return HTTPResponse::fromString(201, "Body goes here",
                                    {{"Location", "http://example.com"}});
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();

  ASSERT_EQ("text/plain", capturedHeader);
  ASSERT_EQ("", capturedBody);
  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(201, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(
      "http://example.com",
      responseHandler.messages[0].getHeaders().getSingleOrEmpty("Location"));
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Body goes here", to_string(responseHandler.bodies[0]));
}

TEST(DISABLED_HTTPHandlerTest, sets_unset_headers) {}
TEST(DISABLED_HTTPHandlerTest, does_not_set_default_headers_if_already_set) {}
TEST(DISABLED_HTTPHandlerTest, drives_future_with_correct_evb) {}
}
}
