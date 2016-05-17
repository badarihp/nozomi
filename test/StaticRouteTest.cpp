#include <gtest/gtest.h>

#include <folly/Format.h>
#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StaticRoute.h"
#include "test/Common.h"

using namespace std;
using folly::Future;
using folly::Optional;
using folly::sformat;
using proxygen::HTTPMethod;
using std::function;

namespace nozomi {
namespace test {

TEST(StaticRouteTest, static_routes_are_static) {
  auto r =
      make_static_route("/", {HTTPMethod::GET},
                        function<Future<HTTPResponse>(const HTTPRequest&)>());
  ASSERT_TRUE(r->isStaticRoute());
}

TEST(StaticRouteTest, static_routes_match_exact_strings) {
  function<Future<HTTPResponse>(const HTTPRequest&)> f;
  auto r1 = make_static_route("/\\w+", {HTTPMethod::GET}, f);
  auto r2 = make_static_route("/testing/route", {HTTPMethod::GET}, f);

  auto request1 = make_request("/\\w+", HTTPMethod::GET);
  auto matches1 = r1->handler(&request1.getRawRequest());
  auto request2 = make_request("/blargl", HTTPMethod::GET);
  auto matches2 = r1->handler(&request2.getRawRequest());
  auto request3 = make_request("/testing/route", HTTPMethod::GET);
  auto matches3 = r2->handler(&request3.getRawRequest());
  auto request4 = make_request("/testing/route/", HTTPMethod::GET);
  auto matches4 = r2->handler(&request4.getRawRequest());

  ASSERT_EQ(RouteMatchResult::RouteMatched, matches1.result);
  ASSERT_EQ(RouteMatchResult::PathNotMatched, matches2.result);
  ASSERT_EQ(RouteMatchResult::RouteMatched, matches3.result);
  ASSERT_EQ(RouteMatchResult::PathNotMatched, matches4.result);
}

TEST(StaticRouteTest, static_routes_reject_invalid_method) {
  function<Future<HTTPResponse>(const HTTPRequest&)> f;
  auto r =
      make_static_route("/testing", {HTTPMethod::GET, HTTPMethod::POST}, f);
  auto request = make_request("/testing", HTTPMethod::PUT);
  auto matches = r->handler(&request.getRawRequest());
  ASSERT_EQ(RouteMatchResult::MethodNotMatched, matches.result);
}

TEST(StaticRouteTest, static_routes_work) {
  auto request = make_request("/testing", HTTPMethod::GET);
  string requestPath = "";

  function<Future<HTTPResponse>(const HTTPRequest&)> f(
      [&requestPath](const HTTPRequest& request) mutable {
        requestPath = request.getPath();
        return HTTPResponse::future(200);
      });

  auto r = make_static_route("/testing", {HTTPMethod::GET}, f);

  auto result = r->handler(&request.getRawRequest());

  ASSERT_EQ(RouteMatchResult::RouteMatched, result.result);
  ASSERT_TRUE((bool)result.handler);
  ASSERT_FALSE((bool)result.streamingHandler);

  result.handler(std::move(request));
  ASSERT_EQ(requestPath, "/testing");
}

struct TestController {
  static Future<HTTPResponse> staticHandler(const HTTPRequest& request) {
    return HTTPResponse::future(200, request.getPath());
  }
};

TEST(StaticRouteTest, static_routes_take_mutable_callables) {
  int i = 0;
  auto request = make_request("/testing", HTTPMethod::GET);
  auto r = make_static_route(
      "/testing", {HTTPMethod::GET}, [&i](const HTTPRequest& request) {
        i = 5;
        return HTTPResponse::future(200, request.getPath());
      });
  auto r3 = make_static_route("/testing", {HTTPMethod::GET},
                              &TestController::staticHandler);

  ASSERT_EQ("/testing", to_string(r->handler(&request.getRawRequest())
                                      .handler(request)
                                      .get()
                                      .getBody()));
  ASSERT_EQ(5, i);
}
TEST(StaticRouteTest, static_routes_take_all_callables) {
  auto request = make_request("/testing", HTTPMethod::GET);
  auto r1 = make_static_route(
      "/testing", {HTTPMethod::GET},
      function<Future<HTTPResponse>(const HTTPRequest&)>(
          [](const HTTPRequest& request) {
            return HTTPResponse::future(200, request.getPath());
          }));
  auto r2 = make_static_route(
      "/testing", {HTTPMethod::GET}, [](const HTTPRequest& request) {
        return HTTPResponse::future(200, request.getPath());
      });
  auto r3 = make_static_route("/testing", {HTTPMethod::GET},
                              &TestController::staticHandler);

  ASSERT_EQ("/testing", to_string(r1->handler(&request.getRawRequest())
                                      .handler(request)
                                      .get()
                                      .getBody()));
  ASSERT_EQ("/testing", to_string(r2->handler(&request.getRawRequest())
                                      .handler(request)
                                      .get()
                                      .getBody()));
  ASSERT_EQ("/testing", to_string(r3->handler(&request.getRawRequest())
                                      .handler(request)
                                      .get()
                                      .getBody()));
}

TEST(StaticRouteTest, static_route_calls_streaming_handler) {
  folly::EventBase evb;
  auto handler = std::make_unique<TestStreamingHandler<>>(&evb);
  auto request = make_request("/testing/1", HTTPMethod::GET);

  auto r = make_streaming_static_route("/testing/1", {HTTPMethod::GET},
                                       [&handler]() { return handler.get(); });

  r->handler(&request.getRawRequest()).streamingHandler();

  ASSERT_TRUE(handler->setRequestArgsCalled);
}

TEST(StaticRouteTest, streaming_handler_returns_nullptr_on_handler_exception) {
  auto request = make_request("/", HTTPMethod::GET);

  auto r = make_streaming_static_route(
      "/", {HTTPMethod::GET},
      []() -> TestStreamingHandler<>* { throw runtime_error("Error"); });

  ASSERT_EQ(nullptr, r->handler(&request.getRawRequest()).streamingHandler());
}

TEST(StaticRouteTest, streaming_handler_returns_nullptr_on_handler_nullptr) {
  auto request = make_request("/", HTTPMethod::GET);

  auto r = make_streaming_static_route(
      "/", {HTTPMethod::GET},
      []() -> TestStreamingHandler<>* { return nullptr; });

  ASSERT_EQ(nullptr, r->handler(&request.getRawRequest()).streamingHandler());
}
}
}
