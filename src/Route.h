#pragma once

#include <boost/regex.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

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
             std::function<HTTPResponse()> handler =
                 std::function<HTTPResponse()>())
      : result(result), handler(std::move(handler)) {}

  const RouteMatchResult result;
  const std::function<HTTPResponse()> handler;
};

class BaseRoute {
 protected:
  std::string originalPattern_;
  std::unordered_set<proxygen::HTTPMethod> methods_;
  bool isStaticRoute_;
  //TODO: This should take a path and a method, not a request

  BaseRoute(std::string originalPattern,
            std::unordered_set<proxygen::HTTPMethod> methods,
            bool isStaticRoute)
      : originalPattern_(std::move(originalPattern)),
        methods_(std::move(methods)),
        isStaticRoute_(isStaticRoute) {}

 public:
  virtual ~BaseRoute() {};
  virtual RouteMatch handler(std::shared_ptr<const HTTPRequest>& request) = 0;
  inline bool isStaticRoute() { return isStaticRoute_; }
};

template <typename... HandlerArgs>
class Route : public virtual BaseRoute {
  // TODO: Streaming caller
 private:
  std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler_;
  boost::basic_regex<char> regex_;

 public:
  Route(std::string pattern,
        std::unordered_set<proxygen::HTTPMethod> methods,
        std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler);
  virtual RouteMatch handler(std::shared_ptr<const HTTPRequest>& request);
};

template <typename... HandlerArgs>
inline std::unique_ptr<Route<HandlerArgs...>> make_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler) {
  return std::make_unique<Route<HandlerArgs...>>(std::move(pattern), std::move(methods),
                              std::move(handler));
}

class StaticRoute : public virtual BaseRoute {
  // TODO: Streaming caller
 private:
  std::function<HTTPResponse(const HTTPRequest&)> handler_;

 public:
  StaticRoute(std::string pattern,
              std::unordered_set<proxygen::HTTPMethod> methods,
              std::function<HTTPResponse(const HTTPRequest& request)> handler);
  virtual RouteMatch handler(std::shared_ptr<const HTTPRequest>& request);
};

inline std::unique_ptr<BaseRoute> make_static_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    std::function<HTTPResponse(const HTTPRequest& request)> handler) {
  return std::make_unique<StaticRoute>(std::move(pattern), std::move(methods),
                     std::move(handler));
}
}

#include "src/Route-inl.h"
