#include <gtest/gtest.h>

#include <chrono>

#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/HTTPHandlerFactory.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/StaticRoute.h"
#include "test/Common.h"

using namespace std;
using namespace folly;
using namespace proxygen;

namespace nozomi {
namespace test {

struct CustomHandler : public proxygen::RequestHandler {
  std::chrono::milliseconds timeout;
  Router* router;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;
  CustomHandler(
      std::chrono::milliseconds timeout,
      Router* router,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler)
      : timeout(timeout), router(router), handler(std::move(handler)) {}
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override {}
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {}
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override {}
  virtual void onEOM() noexcept override {}
  virtual void requestComplete() noexcept override {}
  virtual void onError(proxygen::ProxygenError err) noexcept override {}

  virtual ~CustomHandler() noexcept {}
};

TEST(HTTPHandlerFactoryTest, returns_nonstreaming_handler) {
  EventBase evb;
  auto router = make_router(
      {}, make_static_route("/", {HTTPMethod::GET}, [](const auto&) {
        return HTTPResponse::future(200, "Sample string");
      }));
  Config c({make_tuple("::1", 8080, HTTPServer::Protocol::HTTP)}, 1,
           std::chrono::milliseconds(10000));
  HTTPHandlerFactory<CustomHandler> factory(std::move(c), std::move(router));
  auto request = make_request("/");
  auto rawRequest = request.getRawRequest();

  factory.onServerStart(&evb);
  auto* handlerPtr = factory.onRequest(nullptr, &rawRequest);

  unique_ptr<RequestHandler> handler(handlerPtr);
  auto* castPtr = static_cast<CustomHandler*>(handlerPtr);
  auto response = castPtr->handler(request).get();

  ASSERT_TRUE(handler != nullptr);
  ASSERT_EQ(std::chrono::milliseconds(10000), castPtr->timeout);
  ASSERT_NE(nullptr, castPtr->router);
  ASSERT_EQ("Sample string", response.getBodyString());
}

TEST(HTTPHandlerFactoryTest, returns_streaming_handler) {
  EventBase evb;
  TestStreamingHandler<> streamingHandler;
  auto router =
      make_router({}, make_streaming_static_route(
                          "/", {HTTPMethod::GET},
                          [&streamingHandler]() { return &streamingHandler; }));

  Config c({make_tuple("::1", 8080, HTTPServer::Protocol::HTTP)}, 1,
           std::chrono::milliseconds(10000));
  HTTPHandlerFactory<CustomHandler> factory(std::move(c), std::move(router));
  auto request = make_request("/");
  auto rawRequest = request.getRawRequest();

  factory.onServerStart(&evb);
  auto* handlerPtr = factory.onRequest(nullptr, &rawRequest);

  ASSERT_EQ(&streamingHandler, handlerPtr);
}
}
}
