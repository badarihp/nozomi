#include <gtest/gtest.h>

#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/Common.h"
#include "src/HTTPHandlerFactory.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"
#include "src/StaticRoute.h"

using namespace std;
using namespace folly;
using namespace proxygen;

namespace sakura {
namespace test {

HTTPRequest make_request(
    string path,
    HTTPMethod method = HTTPMethod::GET,
    unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0)) {
  auto headers = std::make_unique<proxygen::HTTPMessage>();
  headers->setURL(path);
  headers->setMethod(method);
  return HTTPRequest(std::move(headers), std::move(body));
}

struct CustomHandler : public proxygen::RequestHandler {
  folly::EventBase* evb_;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler;
  CustomHandler(
      folly::EventBase* evb,
      Router* router,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)> handler)
      : evb_(evb), handler(std::move(handler)) {}
  virtual void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override {}
  virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {}
  virtual void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override {}
  virtual void onEOM() noexcept override {}
  virtual void requestComplete() noexcept override {}
  virtual void onError(proxygen::ProxygenError err) noexcept override {}

  virtual ~CustomHandler() noexcept {}
};

TEST(HTTPHandlerFactoryTest, returns_correct_handler) {
  EventBase evb;
  auto router = make_router(
      {}, make_static_route("/", {HTTPMethod::GET}, [](const auto& r) {
        return HTTPResponse::future(200, "Sample string");
      }));
  Config c({make_tuple("::1", 8080, HTTPServer::Protocol::HTTP)}, 1);
  HTTPHandlerFactory<CustomHandler> factory(std::move(c), std::move(router));
  auto request = make_request("/");
  auto rawRequest = request.getRawRequest();

  factory.onServerStart(&evb);
  auto* handlerPtr = factory.onRequest(nullptr, &rawRequest);
  unique_ptr<RequestHandler> handler(handlerPtr);
  auto response =
      static_cast<CustomHandler*>(handlerPtr)->handler(request).get();

  ASSERT_TRUE(handler != nullptr);
  ASSERT_EQ(&evb, static_cast<CustomHandler*>(handlerPtr)->evb_);
  ASSERT_EQ("Sample string", response.getBodyString());
}

TEST(HTTPHandlerFactoryTest, passes_event_base_through) {
  EventBase evb;
  auto router = make_router(
      {}, make_static_route("/", {HTTPMethod::GET}, [](const auto& r) {
        return HTTPResponse::future(200);
      }));
  Config c({make_tuple("::1", 8080, HTTPServer::Protocol::HTTP)}, 1);
  HTTPHandlerFactory<CustomHandler> factory(std::move(c), std::move(router));
  auto request = make_request("/");
  auto rawRequest = request.getRawRequest();

  factory.onServerStart(&evb);
  auto* handlerPtr = factory.onRequest(nullptr, &rawRequest);
  unique_ptr<RequestHandler> handler(handlerPtr);
  ASSERT_TRUE(handler != nullptr);
  ASSERT_EQ(&evb, static_cast<CustomHandler*>(handlerPtr)->evb_);
}
}
}
