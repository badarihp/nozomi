#include <gtest/gtest.h>

#include <folly/Optional.h>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"

using namespace std;
using folly::Optional;

namespace sakura {
namespace test {

TEST(RouteTest, fails_if_path_has_different_number_of_args_than_func) {
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f1;
  std::function<HTTPResponse(const HTTPRequest&)> f2;
  ASSERT_THROW({ auto r = make_route("/{{i}}", {"GET"}, f1); },
               std::runtime_error);
  ASSERT_THROW({ auto r = make_route("/{{i}}/{{i}}/{{i}}", {"GET"}, f1); },
               std::runtime_error);
  ASSERT_THROW({ auto r = make_route("/", {"GET"}, f1); }, std::runtime_error);
  ASSERT_THROW({ auto r = make_route("/{{i}}", {"GET"}, f2); },
               std::runtime_error);
}

TEST(RouteTest, fails_if_path_has_different_args_than_func) {
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  ASSERT_THROW({ auto r = make_route("/{{i}}/{{d}}", {"GET"}, f); },
               std::runtime_error);
}

template <typename... Args>
void testRouteDoesntMatch(string requestPath, string pattern) {
  HTTPRequest request(requestPath, "GET");
  std::function<HTTPResponse(const HTTPRequest&, Args...)> f(
      [](const HTTPRequest& request, Args... args) { return HTTPResponse(200); });
  auto r = make_route(pattern, {"GET"}, f);

  auto match = r->handler(request);

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
  HTTPRequest request("/1234/1234", "PUT");
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  auto r = make_route("/{{i}}/{{i}}", {"GET", "POST"}, f);
  auto match = r->handler(request);

  ASSERT_EQ(RouteMatchResult::MethodNotMatched, match.result);
  ASSERT_FALSE((bool)match.handler);
}

template <typename... Args>
void testRouteMatching(string requestPath,
                       string pattern,
                       tuple<Args...> expected) {
  std::tuple<Args...> result;
  bool ranFunction = false;
  HTTPRequest request(requestPath, "GET");
  std::function<HTTPResponse(const HTTPRequest&, Args...)> f = [&result,
                                                                &ranFunction](
      const HTTPRequest& request, Args... args) mutable {
    result = std::make_tuple<Args...>(std::move(args)...);
    ranFunction = true;
    return HTTPResponse(200);
  };

  auto route = make_route(pattern, {"GET"}, f);
  auto match = route->handler(request);

  ASSERT_EQ(RouteMatchResult::RouteMatched, match.result) << "Request path: " << requestPath
                                 << " pattern: " << pattern;
  ASSERT_TRUE((bool)match.handler) << "Request path: " << requestPath
                                  << " pattern: " << pattern;

  match.handler(request);
  ASSERT_TRUE(ranFunction) << "Request path: " << requestPath
                           << " pattern: " << pattern;
  ASSERT_EQ(expected, result) << "Request path: " << requestPath
                              << " pattern: " << pattern;
}

TEST(RouteTest, properly_matches_paths) {
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

  std::function<HTTPResponse(const HTTPRequest&, int64_t)> f_int(
      [&int_result](const HTTPRequest& r, int64_t i) mutable {
        int_result = i;
        return HTTPResponse(200);
      });
  std::function<HTTPResponse(const HTTPRequest&, double)> f_double(
      [&double_result](const HTTPRequest& r, double i) mutable {
        double_result = i;
        return HTTPResponse(200);
      });

  HTTPRequest request1("/1777777777777777777777", "GET");
  HTTPRequest request2("/-1777777777777777777777", "GET");
  HTTPRequest request3("/1" + std::to_string(numeric_limits<double>::max()),
                       "GET");
  HTTPRequest request4("/-1" + std::to_string(numeric_limits<double>::max()),
                       "GET");

  auto r_int = make_route("/{{i}}", {"GET"}, f_int);
  auto r_double = make_route("/{{d}}", {"GET"}, f_double);

  int_result = 0;
  r_int->handler(request1).handler(request1);
  ASSERT_EQ(int_result, numeric_limits<int64_t>::max());

  int_result = 0;
  r_int->handler(request2).handler(request2);
  ASSERT_EQ(int_result, numeric_limits<int64_t>::max());

  double_result = 0.0;
  r_double->handler(request3).handler(request3);
  ASSERT_EQ(double_result, numeric_limits<double>::infinity());

  double_result = 0.0;
  r_double->handler(request4).handler(request4);
  ASSERT_EQ(double_result, -numeric_limits<double>::infinity());
}

TEST(RouteTest, invalid_string_regexes_fail) {
  ASSERT_THROW(
      {
        auto r = make_route("/{{i:.+)}}", {"GET"},
                            std::function<HTTPResponse(const HTTPRequest&,
                                                       Optional<int64_t>)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route("/{{d:.+)}}", {"GET"},
                            std::function<HTTPResponse(const HTTPRequest&,
                                                       Optional<double>)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route(
            "/{{s:.+)}}", {"GET"},
            std::function<HTTPResponse(const HTTPRequest&, string)>());
      },
      std::runtime_error);
  ASSERT_THROW(
      {
        auto r = make_route("/{{s?:.+)}}", {"GET"},
                            std::function<HTTPResponse(const HTTPRequest&,
                                                       Optional<string>)>());
      },
      std::runtime_error);
}

TEST(RouteTest, regex_routes_are_not_static) {
  auto r = make_route("/{{i}}", {"GET"}, std::function<HTTPResponse(const HTTPRequest&, int64_t)>());
  ASSERT_FALSE(r->isStaticRoute());
}

TEST(RouteTest, static_routes_are_static) {
  auto r = make_static_route("/", {"GET"}, std::function<HTTPResponse(const HTTPRequest&)>());
  ASSERT_TRUE(r->isStaticRoute());
}

TEST(RouteTest, static_routes_match_exact_strings) {
  std::function<HTTPResponse(const HTTPRequest&)> f;
  auto r1 = make_static_route("/\\w+", {"GET"}, f);
  auto r2 = make_static_route("/testing/route", {"GET"}, f);

  auto matches1 = r1->handler(HTTPRequest("/\\w+", "GET"));
  auto matches2 = r1->handler(HTTPRequest("/blargl", "GET"));
  auto matches3 = r2->handler(HTTPRequest("/testing/route", "GET"));
  auto matches4 = r2->handler(HTTPRequest("/testing/route/", "GET"));

  ASSERT_EQ(RouteMatchResult::RouteMatched, matches1.result);
  ASSERT_EQ(RouteMatchResult::PathNotMatched, matches2.result);
  ASSERT_EQ(RouteMatchResult::RouteMatched, matches3.result);
  ASSERT_EQ(RouteMatchResult::PathNotMatched, matches4.result);
}

TEST(RouteTest, static_routes_reject_invalid_method) {
  std::function<HTTPResponse(const HTTPRequest&)> f;
  auto r = make_static_route("/testing", {"GET", "POST"}, f);
  auto matches = r->handler(HTTPRequest("/testing", "PUT"));
  ASSERT_EQ(RouteMatchResult::MethodNotMatched, matches.result);
}

TEST(RouteTest, static_routes_work) {
  HTTPRequest request("/testing", "GET");
  string requestPath = "";

  std::function<HTTPResponse(const HTTPRequest&)> f(
    [&requestPath](const HTTPRequest& request) mutable {
      requestPath = request.getPath();
      return HTTPResponse(200);
    });
  auto r = make_static_route("/testing", {"GET"}, f);
  r->handler(request).handler(request);
  ASSERT_EQ(requestPath, request.getPath());
}

}
}
