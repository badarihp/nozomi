#include <gtest/gtest.h>

#include <folly/Optional.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Router.h"

#include <string>

using namespace std;
using folly::Optional;

namespace sakura {
namespace test {

// TODO: Flesh out HTTPResponse a bit so the tests can use response bodies,
//      rather than status codes to verify calls
TEST(RouterTest, checks_static_routes_first) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>(
          [](const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/{{i}}", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&, int64_t)>([](
          const HTTPRequest& reqeust, int i) { return HTTPResponse(202); })));

  Router router(std::move(routes), {});
  HTTPRequest request("/1", "GET");

  auto handler = router.getHandler(request);
  auto response = handler(request);
  ASSERT_EQ(201, response.getStatusCode());
}

TEST(RouterTest, returns_405_when_unsupported_method_is_found) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {"GET", "PUT"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(202); })));
  Router router(std::move(routes), {});
  HTTPRequest request("/1", "POST");

  auto handler = router.getHandler(request);
  auto response = handler(request);

  ASSERT_EQ(405, response.getStatusCode());
}

TEST(RouterTest, returns_route_when_static_405d_but_regex_route_didnt) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {"GET", "POST"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(202); })));
  Router router(std::move(routes), {});
  HTTPRequest request("/1", "POST");

  auto handler = router.getHandler(request);
  auto response = handler(request);

  ASSERT_EQ(202, response.getStatusCode());
}

TEST(RouterTest, returns_custom_error_handler_when_set) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {"GET", "PUT"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router(
      std::move(routes),
      {
          {405, [](const auto& request) { return HTTPResponse(415); }},
          {404, [](const auto& request) { return HTTPResponse(410); }},
      });
  HTTPRequest request1("/1", "POST");
  HTTPRequest request2("/invalid_path", "GET");

  auto handler1 = router.getHandler(request1);
  auto response1 = handler1(request1);
  auto handler2 = router.getHandler(request2);
  auto response2 = handler2(request2);

  ASSERT_EQ(415, response1.getStatusCode());
  ASSERT_EQ(410, response2.getStatusCode());
}

TEST(RouterTest, returns_default_error_handler_when_not_set) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {"GET", "PUT"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router(
      std::move(routes),
      {
          {401, [](const auto& request) { return HTTPResponse(411); }},
          {402, [](const auto& request) { return HTTPResponse(412); }},
      });
  HTTPRequest request1("/1", "POST");
  HTTPRequest request2("/invalid_path", "GET");

  auto handler1 = router.getHandler(request1);
  auto response1 = handler1(request1);
  auto handler2 = router.getHandler(request2);
  auto response2 = handler2(request2);

  ASSERT_EQ(405, response1.getStatusCode());
  ASSERT_EQ(404, response2.getStatusCode());
}

TEST(RouterTest, returns_404_when_route_not_found) {
  vector<unique_ptr<BaseRoute>> routes;
  routes.push_back(make_static_route(
      "/1", {"GET"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(201); })));
  routes.push_back(make_route(
      "/\\d+", {"GET", "PUT"},
      std::function<HTTPResponse(const HTTPRequest&)>([](
          const HTTPRequest& request) { return HTTPResponse(202); })));

  Router router(std::move(routes), {});
  HTTPRequest request("/invalid_path", "GET");

  auto handler = router.getHandler(request);
  auto response = handler(request);

  ASSERT_EQ(404, response.getStatusCode());
}

TEST(RouterTest, make_router_works) {
  auto router = make_router(
      {{404, [](const auto& request) { return HTTPResponse(414); }}},
      make_static_route("/1", {"GET"},
                        std::function<HTTPResponse(const HTTPRequest&)>(
                            [](const HTTPRequest& request) {
                              return HTTPResponse(201);
                            })),
      make_route("/2", {"GET"},
                        std::function<HTTPResponse(const HTTPRequest&)>(
                            [](const HTTPRequest& request) {
                              return HTTPResponse(202);
                            }))
  );

  auto request1 = HTTPRequest("/1", "GET");
  auto response1 = router.getHandler(request1)(request1);
  auto request2 = HTTPRequest("/3", "GET");
  auto response2 = router.getHandler(request2)(request2);

  ASSERT_EQ(201, response1.getStatusCode());
  ASSERT_EQ(414, response2.getStatusCode());
}
}
}
