#pragma once

#include <boost/regex.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <folly/Optional.h>

namespace sakura {

enum RouteMatchResult {
  PathNotMatched,    // Path didn't match
  MethodNotMatched,  // Path matched, method didn't
  RouteMatched,      // The route matched entirely
};

/*
 * Determines whether a Route matches a given pattern, and if not,
 * why it doesn't
 */
struct RouteMatch {
  RouteMatch(RouteMatchResult result,
             std::function<HTTPResponse(const HTTPRequest&)> handler =
                 std::function<HTTPResponse(const HTTPRequest&)>())
      : result(result), handler(std::move(handler)) {}

  const RouteMatchResult result;
  const std::function<HTTPResponse(const HTTPRequest&)> handler;
};

class BaseRoute {
 protected:
  std::string originalPattern_;
  std::unordered_set<std::string> methods_;
  bool isStaticRoute_;
  std::function<RouteMatch(const HTTPRequest&)> handler_;

  BaseRoute(std::string originalPattern,
            std::unordered_set<std::string> methods,
            bool isStaticRoute)
      : originalPattern_(std::move(originalPattern)),
        methods_(std::move(methods)),
        isStaticRoute_(isStaticRoute) {}

 public:
  RouteMatch handler(const HTTPRequest& request);
  inline bool isStaticRoute() { return isStaticRoute_; }
};

template <typename... ActionArgs>
class Route : public virtual BaseRoute {
  // TODO: Streaming caller
 private:
  boost::basic_regex<char> regex_;

 public:
  Route(std::string pattern,
        std::unordered_set<std::string> methods,
        std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)> handler);
};

template <typename... ActionArgs>
inline Route<ActionArgs...> make_route(
    std::string pattern,
    std::unordered_set<std::string> methods,
    std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)> handler) {
  return Route<ActionArgs...>(std::move(pattern), std::move(methods),
                              std::move(handler));
}

class StaticRoute : public virtual BaseRoute {
  // TODO: Streaming caller
 public:
  StaticRoute(std::string pattern,
              std::unordered_set<std::string> methods,
              std::function<HTTPResponse(const HTTPRequest& request)> handler);
};

inline StaticRoute make_static_route(
    std::string pattern,
    std::unordered_set<std::string> methods,
    std::function<HTTPResponse(const HTTPRequest& request)> handler) {
  return StaticRoute(std::move(pattern), std::move(methods),
                     std::move(handler));
}
}

#include "src/Route-inl.h"
