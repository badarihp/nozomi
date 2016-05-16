#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/HTTPHandler.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/StaticRoute.h"
#include "src/StringUtils.h"
#include "test/Common.h"

using folly::IOBuf;

using namespace std;
using namespace proxygen;

namespace nozomi {
namespace test {


struct HTTPHandlerTest : public ::testing::Test {
  function<folly::Future<HTTPResponse>(const HTTPRequest&)> errorHandler;
  function<folly::Future<HTTPResponse>(const HTTPRequest&)> timeoutHandler;
  function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;

  std::unique_ptr<HTTPMessage> requestMessage;
  std::unique_ptr<IOBuf> body;
  folly::EventBase evb;
  Router router;
  HTTPHandler httpHandler;
  TestResponseHandler responseHandler;

  HTTPHandlerTest()
      : requestMessage(std::make_unique<HTTPMessage>()),
        body(IOBuf::copyBuffer("the body")),
        router(make_router(
            {{500, function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                       [this](const HTTPRequest& request) {
                         return errorHandler(request);
                       })},
             {503, function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                       [this](const HTTPRequest& request) {
                         return timeoutHandler(request);
                       })}},
            make_route(
                "/",
                {proxygen::HTTPMethod::GET},
                function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
                    [this](const HTTPRequest& request) {
                      return handler(request);
                    })))),
        httpHandler(
            std::chrono::milliseconds(50),
            &router,
            [this](const HTTPRequest& request) { return handler(request); },
            &evb,
            &evb),
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
  evb.loop();

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
  evb.loop();

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
  evb.loop();

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
  evb.loop();

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

TEST_F(HTTPHandlerTest, sends_generic_500_when_custom_503_handler_throws) {
  bool called = false;

  timeoutHandler = [](const HTTPRequest& request) -> HTTPResponse {
    throw std::runtime_error("Broken");
  };
  handler = [&](const HTTPRequest& request) {
    called = true;
    this_thread::sleep_for(chrono::milliseconds(60));
    return HTTPResponse::fromString(201, "Body goes here",
                                    {{"Location", "http://example.com"}});
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_TRUE(called);
  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(500, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Unknown error", to_string(responseHandler.bodies[0]));
}

TEST_F(HTTPHandlerTest, returns_503_on_future_timeout) {
  bool called = false;

  timeoutHandler = [](const HTTPRequest& request) {
    return HTTPResponse::future(503, "Timed out!");
  };
  handler = [&](const HTTPRequest& request) {
    called = true;
    this_thread::sleep_for(chrono::milliseconds(60));
    return HTTPResponse::fromString(201, "Body goes here",
                                    {{"Location", "http://example.com"}});
  };

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_TRUE(called);
  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(503, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Timed out!", to_string(responseHandler.bodies[0]));
}

TEST(DISABLED_HTTPHandlerTest, sets_unset_headers) {}
TEST(DISABLED_HTTPHandlerTest, does_not_set_default_headers_if_already_set) {}
TEST(DISABLED_HTTPHandlerTest, drives_future_with_correct_evb) {}
}
}
