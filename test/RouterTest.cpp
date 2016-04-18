#include <gtest/gtest.h>

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"

#include <string>

using namespace std;
using folly::Optional;
using proxygen::HTTPMethod;

namespace sakura {
namespace test {

shared_ptr<const HTTPRequest> make_request(
    string path,
    HTTPMethod method,
    unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0)) {
  proxygen::HTTPMessage headers;
  headers.setURL(path);
  headers.setMethod(method);
  return std::make_shared<const HTTPRequest>(std::move(headers),
                                             std::move(body));
}

// TODO: Flesh out HTTPResponse a bit so the tests can use response bodies,
//      rather than status codes to verify calls
TEST(RouterTest, checks_static_routes_first) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/{{i}}", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&, int64_t)>([](
          const HTTPRequest& reqeust, int i) { return HTTPResponse(202); })));

  Router router({}, std::move(routes));
  auto request = make_request("/1", HTTPMethod::GET);

  auto handler = router.getHandler(request);
  auto response = handler();
  ASSERT_EQ(201, response.getStatusCode());
}

TEST(RouterTest, returns_405_when_unsupported_method_is_found) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {HTTPMethod::GET, HTTPMethod::PUT},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(202); })));
  Router router({}, std::move(routes));
  auto request = make_request("/1", HTTPMethod::POST);

  auto handler = router.getHandler(request);
  auto response = handler();

  ASSERT_EQ(405, response.getStatusCode());
}

TEST(RouterTest, returns_route_when_static_405d_but_regex_route_didnt) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {HTTPMethod::GET, HTTPMethod::POST},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(202); })));
  Router router({}, std::move(routes));
  auto request = make_request("/1", HTTPMethod::POST);

  auto handler = router.getHandler(request);
  auto response = handler();

  ASSERT_EQ(202, response.getStatusCode());
}

TEST(RouterTest, returns_custom_error_handler_when_set) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {HTTPMethod::GET, HTTPMethod::PUT},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router(
      {
          {405, [](const auto& request) { return HTTPResponse(415); }},
          {404, [](const auto& request) { return HTTPResponse(410); }},
      },
      std::move(routes) );
  auto request1 = make_request("/1", HTTPMethod::POST);
  auto request2 = make_request("/invalid_path", HTTPMethod::GET);

  auto handler1 = router.getHandler(request1);
  auto response1 = handler1();
  auto handler2 = router.getHandler(request2);
  auto response2 = handler2();

  ASSERT_EQ(415, response1.getStatusCode());
  ASSERT_EQ(410, response2.getStatusCode());
}

TEST(RouterTest, returns_default_error_handler_when_not_set) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {HTTPMethod::GET, HTTPMethod::PUT},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router(
      {
          {401, [](const auto& request) { return HTTPResponse(411); }},
          {402, [](const auto& request) { return HTTPResponse(412); }},
      },
      std::move(routes));
  auto request1 = make_request("/1", HTTPMethod::POST);
  auto request2 = make_request("/invalid_path", HTTPMethod::GET);

  auto handler1 = router.getHandler(request1);
  auto response1 = handler1();
  auto handler2 = router.getHandler(request2);
  auto response2 = handler2();

  ASSERT_EQ(405, response1.getStatusCode());
  ASSERT_EQ(404, response2.getStatusCode());
}

TEST(RouterTest, returns_404_when_route_not_found) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {HTTPMethod::GET},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {HTTPMethod::GET, HTTPMethod::PUT},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router({}, std::move(routes));
  auto request = make_request("/invalid_path", HTTPMethod::GET);

  auto handler = router.getHandler(request);
  auto response = handler();

  ASSERT_EQ(404, response.getStatusCode());
}

TEST(RouterTest, make_router_works) {
  auto router = make_router(
      {{404, [](const auto& request) { return HTTPResponse(414); }}},
      make_static_route(
          "/1", {HTTPMethod::GET},
          std::function<HTTPResponse(const HTTPRequest&)>(
              [](const HTTPRequest& request) { return HTTPResponse(201); })),
      make_route(
          "/2", {HTTPMethod::GET},
          std::function<HTTPResponse(const HTTPRequest&)>(
              [](const HTTPRequest& request) { return HTTPResponse(202); })));

  auto request1 = make_request("/1", HTTPMethod::GET);
  auto response1 = router.getHandler(request1)();
  auto request2 = make_request("/3", HTTPMethod::GET);
  auto response2 = router.getHandler(request2)();

  ASSERT_EQ(201, response1.getStatusCode());
  ASSERT_EQ(414, response2.getStatusCode());
}
}
}
