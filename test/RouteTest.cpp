#include <gtest/gtest.h>

#include <folly/Format.h>
#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "test/Common.h"

using namespace std;
using folly::Future;
using folly::Optional;
using folly::sformat;
using proxygen::HTTPMethod;
using std::function;

namespace nozomi {
namespace test {

struct TestController {
  static Future<HTTPResponse> noArgHandler(const HTTPRequest& request) {
    return HTTPResponse::future(200, request.getPath());
  }
  static Future<HTTPResponse> dynamicHandler(const HTTPRequest& request,
                                             int64_t i) {
    return HTTPResponse::future(200, sformat("{} {}", request.getPath(), i));
  }
};

TEST(RouteTest, fails_if_path_has_different_number_of_args_than_func) {
  function<Future<HTTPResponse>(const HTTPRequest&, int64_t, int64_t)> f1;
  function<Future<HTTPResponse>(const HTTPRequest&)> f2;
  ASSERT_THROW_MSG(
      { auto r = make_route("/{{i}}", {HTTPMethod::GET}, f1); },
      std::runtime_error,
      "Pattern parameter count != function parameter count (1 vs 2)");
  ASSERT_THROW_MSG(
      { auto r = make_route("/{{i}}/{{i}}/{{i}}", {HTTPMethod::GET}, f1); },
      std::runtime_error,
      "Pattern parameter count != function parameter count (3 vs 2)");
  ASSERT_THROW_MSG(
      { auto r = make_route("/", {HTTPMethod::GET}, f1); }, std::runtime_error,
      "Pattern parameter count != function parameter count (0 vs 2)");
  ASSERT_THROW_MSG(
      { auto r = make_route("/{{i}}", {HTTPMethod::GET}, f2); },
      std::runtime_error,
      "Pattern parameter count != function parameter count (1 vs 0)");
}

TEST(RouteTest, fails_if_path_has_different_args_than_func) {
  function<Future<HTTPResponse>(const HTTPRequest&, int64_t, int64_t)> f;
  ASSERT_THROW_MSG(
      { auto r = make_route("/{{i}}/{{d}}", {HTTPMethod::GET}, f); },
      std::runtime_error,
      ".*Pattern parameter .* (.*) does not match function parameter.*");
}

template <typename... Args>
void testRouteDoesntMatch(string requestPath, string pattern) {
  auto request = make_request(requestPath, HTTPMethod::GET);
  auto f = [](const HTTPRequest& request, Args... args) {
    return HTTPResponse::future(200);
  };
  auto r = make_route(pattern, {HTTPMethod::GET}, f);

  auto match = r->handler(&request.getRawRequest());

  ASSERT_EQ(RouteMatchResult::PathNotMatched, match.result);
  ASSERT_FALSE((bool)match.handler);
}

TEST(RouteTest, returns_empty_function_if_pattern_doesnt_match) {
  // TODO: Tests on more types
  testRouteDoesntMatch<int64_t>("/1234.5", "/{{i}}");
  testRouteDoesntMatch<double>("/1234.5.6", "/{{d}}");
  testRouteDoesntMatch<string>("/1234.5.6", "/{{s:\\w+}}");
  testRouteDoesntMatch<Optional<int64_t>>("/12345/", "/{{i?}}");
  testRouteDoesntMatch<Optional<double>>("/1234.5/", "/{{d?}}");
  testRouteDoesntMatch<Optional<string>>("/testing/", "/{{s?:\\w+}}");
  testRouteDoesntMatch<Optional<int64_t>>("/12345/x/", "/{{i?:/}}");
  testRouteDoesntMatch<Optional<double>>("/1234.5/x/", "/{{d?:/}}");
  testRouteDoesntMatch<Optional<string>>("/testing/x/", "/{{s?:\\w+:/}}");
}

TEST(RouteTest, returns_invalid_method_if_pattern_matches_and_methods_dont) {
  auto request = make_request("/1234/1234", HTTPMethod::PUT);
  function<Future<HTTPResponse>(const HTTPRequest&, int64_t, int64_t)> f;
  auto r = make_route("/{{i}}/{{i}}", {HTTPMethod::GET, HTTPMethod::POST}, f);
  auto match = r->handler(&request.getRawRequest());

  ASSERT_EQ(RouteMatchResult::MethodNotMatched, match.result);
  ASSERT_FALSE((bool)match.handler);
}

template <typename... Args>
void testRouteMatching(string requestPath,
                       string pattern,
                       tuple<Args...> expected) {
  std::tuple<Args...> result;
  bool ranFunction = false;
  auto request = make_request(requestPath, HTTPMethod::GET);
  auto f = [&result, &ranFunction](const HTTPRequest& request,
                                   Args... args) mutable {
    result = std::make_tuple<Args...>(std::move(args)...);
    ranFunction = true;
    return HTTPResponse::future(200);
  };

  auto route = make_route(pattern, {HTTPMethod::GET}, f);
  auto match = route->handler(&request.getRawRequest());

  ASSERT_EQ(RouteMatchResult::RouteMatched, match.result)
      << "Request path: " << requestPath << " pattern: " << pattern;
  ASSERT_TRUE((bool)match.handler) << "Request path: " << requestPath
                                   << " pattern: " << pattern;

  match.handler(std::move(request));
  ASSERT_TRUE(ranFunction) << "Request path: " << requestPath
                           << " pattern: " << pattern;
  ASSERT_EQ(expected, result) << "Request path: " << requestPath
                              << " pattern: " << pattern;
}

TEST(RouteTest, properly_matches_paths) {
  testRouteMatching("/1234", "/\\d+", make_tuple<>());
  testRouteMatching("/1234", "/{{i}}", make_tuple<int64_t>(1234));
  testRouteMatching("/-1234", "/{{i}}", make_tuple<int64_t>(-1234));
  testRouteMatching("/+1234", "/{{i}}", make_tuple<int64_t>(1234));

  testRouteMatching("/1234.5", "/{{d}}", make_tuple<double>(1234.5));
  testRouteMatching("/-1234.5", "/{{d}}", make_tuple<double>(-1234.5));
  testRouteMatching("/+1234.5", "/{{d}}", make_tuple<double>(1234.5));
  testRouteMatching("/1234", "/{{d}}", make_tuple<double>(1234.0));
  testRouteMatching("/-1234", "/{{d}}", make_tuple<double>(-1234.0));
  testRouteMatching("/+1234", "/{{d}}", make_tuple<double>(1234.0));

  testRouteMatching("/testing1236", R"(/{{s:\w+[0-5]{3}[6-9]?}})",
                    make_tuple<string>("testing1236"));

  testRouteMatching("/", "/{{i?}}",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>()));
  testRouteMatching("/", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>()));
  testRouteMatching("/", R"(/{{s?:\w+[0-5]{3}[6-9]?}})",
                    make_tuple<Optional<string>>(Optional<string>()));

  testRouteMatching("/1/x/", "/{{i?:/}}\\w+/?",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>(1)));
  testRouteMatching("/1.5/x/", "/{{d?:/}}\\w+/?",
                    make_tuple<Optional<double>>(Optional<double>(1.5)));
  testRouteMatching("/y/x/", "/{{s?:y+:/}}\\w+/?",
                    make_tuple<Optional<string>>(Optional<string>("y")));
  testRouteMatching("/x/", "/{{i?:/}}\\w+/?",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>()));
  testRouteMatching("/x/", "/{{d?:/}}\\w+/?",
                    make_tuple<Optional<double>>(Optional<double>()));
  testRouteMatching("/x/", "/{{s?:y+:/}}\\w+/?",
                    make_tuple<Optional<string>>(Optional<string>()));

  testRouteMatching(
      "/1", "/{{i?:/}}{{i}}",
      make_tuple<Optional<int64_t>, int64_t>(Optional<int64_t>(), 1));
  testRouteMatching(
      "/1.5", "/{{d?:/}}{{d}}",
      make_tuple<Optional<double>, double>(Optional<double>(), 1.5));
  testRouteMatching(
      "/testing", "/{{s?:\\w+:/}}{{s:\\w+}}",
      make_tuple<Optional<string>, string>(Optional<string>(), "testing"));

  testRouteMatching("/1234", "/{{i?}}",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>(1234)));
  testRouteMatching("/-1234", "/{{i?}}",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>(-1234)));
  testRouteMatching("/+1234", "/{{i?}}",
                    make_tuple<Optional<int64_t>>(Optional<int64_t>(1234)));

  testRouteMatching("/1234.5", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(1234.5)));
  testRouteMatching("/-1234.5", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(-1234.5)));
  testRouteMatching("/+1234.5", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(1234.5)));
  testRouteMatching("/1234", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(1234.0)));
  testRouteMatching("/-1234", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(-1234.0)));
  testRouteMatching("/+1234", "/{{d?}}",
                    make_tuple<Optional<double>>(Optional<double>(1234.0)));

  testRouteMatching(
      "/testing1236", R"(/{{s?:\w+[0-5]{3}[6-9]?}})",
      make_tuple<Optional<string>>(Optional<string>("testing1236")));

  testRouteMatching(
      "/1/-2/1.5/-2.5/testing1235/other/3/3.5/last",
      R"(/{{i}}/{{i?:/}}{{d}}/{{d?:/}}{{s:\w+}}/{{s?:\w+:/}}{{i?:/}}{{d?:/}}{{s?:\w+:/}}{{i}}/{{d}}/{{s:\w+}})",
      make_tuple<int64_t, Optional<int64_t>, double, Optional<double>, string,
                 Optional<string>, Optional<int64_t>, Optional<double>,
                 Optional<string>, int64_t, double, string>(
          1, Optional<int64_t>(-2), 1.5, Optional<double>(-2.5), "testing1235",
          Optional<string>("other"), Optional<int64_t>(), Optional<double>(),
          Optional<string>(), 3, 3.5, "last"));
}

TEST(RouteTest, overflow_is_handled) {
  int64_t int_result = 0;
  double double_result = 0.0;

  function<Future<HTTPResponse>(const HTTPRequest&, int64_t)> f_int(
      [&int_result](const HTTPRequest& r, int64_t i) mutable {
        int_result = i;
        return HTTPResponse::future(200);
      });
  function<Future<HTTPResponse>(const HTTPRequest&, double)> f_double(
      [&double_result](const HTTPRequest& r, double i) mutable {
        double_result = i;
        return HTTPResponse::future(200);
      });

  auto request1 = make_request("/1777777777777777777777", HTTPMethod::GET);
  auto request2 = make_request("/-1777777777777777777777", HTTPMethod::GET);
  auto request3 = make_request(
      "/1" + std::to_string(numeric_limits<double>::max()), HTTPMethod::GET);
  auto request4 = make_request(
      "/-1" + std::to_string(numeric_limits<double>::max()), HTTPMethod::GET);

  auto r_int = make_route("/{{i}}", {HTTPMethod::GET}, f_int);
  auto r_double = make_route("/{{d}}", {HTTPMethod::GET}, f_double);

  int_result = 0;
  r_int->handler(&request1.getRawRequest()).handler(std::move(request1));
  ASSERT_EQ(int_result, numeric_limits<int64_t>::max());

  int_result = 0;
  r_int->handler(&request2.getRawRequest()).handler(std::move(request2));
  ASSERT_EQ(int_result, numeric_limits<int64_t>::max());

  double_result = 0.0;
  r_double->handler(&request3.getRawRequest()).handler(std::move(request3));
  ASSERT_EQ(double_result, numeric_limits<double>::infinity());

  double_result = 0.0;
  r_double->handler(&request4.getRawRequest()).handler(std::move(request4));
  ASSERT_EQ(double_result, -numeric_limits<double>::infinity());
}

TEST(RouteTest, invalid_string_regexes_fail) {
  ASSERT_THROW(
      {
        auto r = make_route("/{{i:.+)}}", {HTTPMethod::GET},
                            function<Future<HTTPResponse>(
                                const HTTPRequest&, Optional<int64_t>)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route("/{{d:.+)}}", {HTTPMethod::GET},
                            function<Future<HTTPResponse>(const HTTPRequest&,
                                                          Optional<double>)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route(
            "/{{s:.+)}}", {HTTPMethod::GET},
            function<Future<HTTPResponse>(const HTTPRequest&, string)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route("/{{s?:.+)}}", {HTTPMethod::GET},
                            function<Future<HTTPResponse>(const HTTPRequest&,
                                                          Optional<string>)>());
      },
      std::runtime_error);
}

TEST(RouteTest, regex_routes_are_not_static) {
  auto r =
      make_route("/{{i}}", {HTTPMethod::GET},
                 function<Future<HTTPResponse>(const HTTPRequest&, int64_t)>());
  ASSERT_FALSE(r->isStaticRoute());
}

TEST(RouteTest, dynamic_routes_take_mutable_callables) {
  int j = 0;
  auto request = make_request("/testing/1", HTTPMethod::GET);
  auto r = make_route("/testing/{{i}}", {HTTPMethod::GET},
                      [&j](const HTTPRequest& request, int64_t i) mutable {
                        j = 5;
                        return HTTPResponse::future(
                            200, sformat("{} {}", request.getPath(), i));
                      });

  ASSERT_EQ("/testing/1 1", to_string(r->handler(&request.getRawRequest())
                                          .handler(request)
                                          .get()
                                          .getBody()));
  ASSERT_EQ(5, j);
}

TEST(RouteTest, dynamic_routes_take_all_callables) {
  auto request = make_request("/testing/1", HTTPMethod::GET);
  auto r1 =
      make_route("/testing/{{i}}", {HTTPMethod::GET},
                 function<Future<HTTPResponse>(const HTTPRequest&, int64_t)>(
                     [](const HTTPRequest& request, int64_t i) {
                       return HTTPResponse::future(
                           200, sformat("{} {}", request.getPath(), i));
                     }));
  auto r2 = make_route("/testing/{{i}}", {HTTPMethod::GET},
                       [](const HTTPRequest& request, int64_t i) {
                         return HTTPResponse::future(
                             200, sformat("{} {}", request.getPath(), i));
                       });
  auto r3 = make_route("/testing/{{i}}", {HTTPMethod::GET},
                       &TestController::dynamicHandler);

  ASSERT_EQ("/testing/1 1", to_string(r1->handler(&request.getRawRequest())
                                          .handler(request)
                                          .get()
                                          .getBody()));
  ASSERT_EQ("/testing/1 1", to_string(r2->handler(&request.getRawRequest())
                                          .handler(request)
                                          .get()
                                          .getBody()));
  ASSERT_EQ("/testing/1 1", to_string(r3->handler(&request.getRawRequest())
                                          .handler(request)
                                          .get()
                                          .getBody()));
}

TEST(RouteTest, streaming_handlers_are_called_with_correct_args) {
  auto handler1 = std::make_unique<TestStreamingHandler<int64_t>>();
  auto handler2 = std::make_unique<TestStreamingHandler<>>();
  auto request1 = make_request("/testing/1", HTTPMethod::GET);
  auto request2 = make_request("/testing/1", HTTPMethod::GET);

  auto r1 = make_streaming_route("/testing/{{i}}", {HTTPMethod::GET},
                                 [&handler1]() { return handler1.get(); });
  auto r2 = make_streaming_route("/testing/1", {HTTPMethod::GET},
                                 [&handler2]() { return handler2.get(); });

  r1->handler(&request1.getRawRequest()).streamingHandler();
  r2->handler(&request2.getRawRequest()).streamingHandler();

  ASSERT_TRUE(handler1->setRequestArgsCalled);
  ASSERT_TRUE(handler2->setRequestArgsCalled);
  ASSERT_EQ(1, std::get<0>(handler1->requestArgs));
}
}
}
