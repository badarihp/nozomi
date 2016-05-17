#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StreamingHTTPHandler.h"
#include "src/StringUtils.h"
#include "test/Common.h"

using folly::IOBuf;

using namespace std;
using namespace proxygen;

namespace nozomi {
namespace test {

struct StreamingHTTPHandlerTest : public ::testing::Test {
  std::unique_ptr<proxygen::HTTPMessage> requestMessage;
  std::unique_ptr<IOBuf> body;
  folly::EventBase evb;
  TestStreamingHandler<> httpHandler;
  TestResponseHandler responseHandler;

  StreamingHTTPHandlerTest() : httpHandler(&evb), responseHandler(&httpHandler) {
    requestMessage = std::make_unique<HTTPMessage>();
    requestMessage->getHeaders().set("Content-Type", "text/plain");
    requestMessage->setMethod(proxygen::HTTPMethod::GET);
    requestMessage->setURL("/");
    httpHandler.setResponseHandler(&responseHandler);
  }
};

TEST_F(StreamingHTTPHandlerTest, onRequest_sends_request) {
  httpHandler.onRequest(std::move(requestMessage));
  
  ASSERT_TRUE(httpHandler.onRequestReceivedCalled);
  ASSERT_EQ("text/plain",
            httpHandler.request.getHeaders().getSingleOrEmpty("Content-Type"));
}

TEST_F(StreamingHTTPHandlerTest, sendResponseHeaders_sends_headers) {
  HTTPResponse response(205, "first part");
  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.sendResponseHeaders(std::move(response));
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(205, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("first part", to_string(responseHandler.bodies[0]));
}

TEST_F(StreamingHTTPHandlerTest, sendBody_sends_body_without_EOM) {
  HTTPResponse response(205, "first part");
  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.sendResponseHeaders(std::move(response));
  httpHandler.sendBody(IOBuf::copyBuffer("test body"));
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(2, responseHandler.bodies.size());
  ASSERT_EQ(205, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ("first part", to_string(responseHandler.bodies[0]));
  ASSERT_EQ("test body", to_string(responseHandler.bodies[1]));

}

TEST_F(StreamingHTTPHandlerTest, onEOF_sends_EOM) {
  HTTPResponse response(205, "first part");
  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.sendResponseHeaders(std::move(response));
  httpHandler.sendBody(IOBuf::copyBuffer("test body"));
  httpHandler.sendEOF();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(2, responseHandler.bodies.size());
  ASSERT_EQ(205, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ("first part", to_string(responseHandler.bodies[0]));
  ASSERT_EQ("test body", to_string(responseHandler.bodies[1]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
} 

TEST_F(StreamingHTTPHandlerTest, sendResponseHeaders_doesnt_send_empty_body) {
  HTTPResponse response(205);
  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.sendResponseHeaders(std::move(response));
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(205, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());

}

TEST_F(StreamingHTTPHandlerTest, sendBody_does_not_send_empty_body) {
  HTTPResponse response(205, "first part");
  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.sendResponseHeaders(std::move(response));
  httpHandler.sendBody(IOBuf::create(0));
  httpHandler.sendBody(IOBuf::copyBuffer("test body"));
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(2, responseHandler.bodies.size());
  ASSERT_EQ(205, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ("first part", to_string(responseHandler.bodies[0]));
  ASSERT_EQ("test body", to_string(responseHandler.bodies[1]));

}

}
}
