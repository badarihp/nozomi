#pragma once

#include <string>
#include <unordered_set>

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
             std::function<HTTPResponse(const HTTPRequest&)> handler =
                 std::function<HTTPResponse(const HTTPRequest&)>())
      : result(result), handler(std::move(handler)) {}

  const RouteMatchResult result;
  const std::function<HTTPResponse(const HTTPRequest&)> handler;
};

class BaseRoute {
 protected:
  std::string originalPattern_;
  std::unordered_set<proxygen::HTTPMethod> methods_;
  bool isStaticRoute_;
  // TODO: This should take a path and a method, not a request

  BaseRoute(std::string originalPattern,
            std::unordered_set<proxygen::HTTPMethod> methods,
            bool isStaticRoute)
      : originalPattern_(std::move(originalPattern)),
        methods_(std::move(methods)),
        isStaticRoute_(isStaticRoute) {}

 public:
  virtual ~BaseRoute(){};
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) = 0;
  inline bool isStaticRoute() { return isStaticRoute_; }
};
}
