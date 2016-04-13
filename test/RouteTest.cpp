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
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  ASSERT_THROW({ auto r = make_route("/{{i}}", {"GET"}, f); },
               std::runtime_error);

  ASSERT_THROW({ auto r = make_route("/{{i}}/{{i}}/{{i}}", {"GET"}, f); },
               std::runtime_error);
}

TEST(RouteTest, fails_if_path_has_different_args_than_func) {
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  ASSERT_THROW({ auto r = make_route("/{{i}}/{{d}}", {"GET"}, f); }, std::runtime_error);
}

TEST(RouteTest, returns_empty_function_if_pattern_doesnt_match) {
  //TODO: Tests on more types
  HTTPRequest request("/testing", "GET");
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  auto r = make_route("/{{i}}/{{i}}", {"GET"}, f);
  auto match = r.action(request);

  ASSERT_FALSE(match.matchedPath);
  ASSERT_FALSE(match.methodAllowed);
  ASSERT_FALSE((bool)match.action);
}

TEST(RouteTest, returns_invalid_method_if_pattern_matches_and_methods_dont) {
  HTTPRequest request("/1234/1234", "PUT");
  std::function<HTTPResponse(const HTTPRequest&, int64_t, int64_t)> f;
  auto r = make_route("/{{i}}/{{i}}", {"GET", "POST"}, f);
  auto match = r.action(request);

  ASSERT_TRUE(match.matchedPath);
  ASSERT_FALSE(match.methodAllowed);
  ASSERT_FALSE((bool)match.action);
}

template<typename... Args>
void testRouteMatching(string requestPath, string pattern, tuple<Args...> expected) {
  std::tuple<Args...> result;
  bool ranFunction = false;
  HTTPRequest request(requestPath, "GET");
  std::function<HTTPResponse(const HTTPRequest&, Args...)> f =
    [&result, &ranFunction](const HTTPRequest& request, Args... args) mutable {
      result = std::make_tuple<Args...>(std::move(args)...);
      ranFunction = true;
      return HTTPResponse();
    };

  auto route = make_route(pattern, {"GET"}, f);
  auto match = route.action(request);

  ASSERT_TRUE(match.matchedPath) << "Request path: " << requestPath << " pattern: " << pattern;
  ASSERT_TRUE(match.methodAllowed) << "Request path: " << requestPath << " pattern: " << pattern;
  ASSERT_TRUE((bool)match.action) << "Request path: " << requestPath << " pattern: " << pattern;

  match.action(request);
  ASSERT_TRUE(ranFunction) << "Request path: " << requestPath << " pattern: " << pattern;
  ASSERT_EQ(expected, result) << "Request path: " << requestPath << " pattern: " << pattern;
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

  testRouteMatching("/testing1236", R"(/{{s:\w+[0-5]{3}[6-9]?}})", make_tuple<string>("testing1236"));

  testRouteMatching("/", "/{{i?}}", make_tuple<Optional<int64_t>>(Optional<int64_t>()));
  testRouteMatching("/", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>()));
  testRouteMatching("/", R"(/{{s?:\w+[0-5]{3}[6-9]?}})", make_tuple<Optional<string>>(Optional<string>()));

  testRouteMatching("/1234", "/{{i?}}", make_tuple<Optional<int64_t>>(Optional<int64_t>(1234)));
  testRouteMatching("/-1234", "/{{i?}}", make_tuple<Optional<int64_t>>(Optional<int64_t>(-1234)));
  testRouteMatching("/+1234", "/{{i?}}", make_tuple<Optional<int64_t>>(Optional<int64_t>(1234)));

  testRouteMatching("/1234.5", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(1234.5)));
  testRouteMatching("/-1234.5", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(-1234.5)));
  testRouteMatching("/+1234.5", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(1234.5)));
  testRouteMatching("/1234", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(1234.0)));
  testRouteMatching("/-1234", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(-1234.0)));
  testRouteMatching("/+1234", "/{{d?}}", make_tuple<Optional<double>>(Optional<double>(1234.0)));

  testRouteMatching("/testing1236", R"(/{{s?:\w+[0-5]{3}[6-9]?}})", make_tuple<Optional<string>>(Optional<string>("testing1236")));

  testRouteMatching(
    "/1/1.5/-2/-2.5/testing1235/other",
    R"(/{{i}}/{{i?}}/{{d}}/{{d?}}/{{s:\w+}}/{{s?:\w+}}/{{i?}}/{{d?}}/{{s?:\w+}}")",
    make_tuple<int64_t, Optional<int64_t>, double, Optional<double>, string, Optional<string>, Optional<int64_t>, Optional<double>, Optional<string>>(
      1, Optional<int64_t>(1.5), -2, Optional<double>(-2.5), "testing1235", Optional<string>("other"), Optional<int64_t>(), Optional<double>(), Optional<string>())
    );
}

TEST(RouteTest, returns_method_that_calls_with_correctly_parsed_args) {}
}
}
