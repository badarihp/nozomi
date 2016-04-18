#include "src/Router.h"

using std::vector;
using std::unique_ptr;
using std::unordered_map;

namespace sakura {

Router::Router(
    unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
        errorRoutes,
    vector<unique_ptr<BaseRoute>> routes)
    : errorRoutes_(std::move(errorRoutes)) {
  for (auto& route : routes) {
    if (route->isStaticRoute()) {
      staticRoutes_.push_back(std::move(route));
    } else {
      routes_.push_back(std::move(route));
    }
  }
}

std::function<HTTPResponse()> Router::getHandler(
    std::shared_ptr<const HTTPRequest>& request) {
  // Check static routes first, then dynamic ones
  bool methodNotFound = false;
  for (const auto& route : staticRoutes_) {
    auto match = route->handler(request);
    switch (match.result) {
      case (RouteMatchResult::PathNotMatched):
        continue;
      case (RouteMatchResult::MethodNotMatched):
        methodNotFound = true;
        break;
      case (RouteMatchResult::RouteMatched):
        return std::move(match.handler);
    }
  }

  for (const auto& route : routes_) {
    auto match = route->handler(request);
    switch (match.result) {
      case (RouteMatchResult::PathNotMatched):
        continue;
      case (RouteMatchResult::MethodNotMatched):
        methodNotFound = true;
        break;
      case (RouteMatchResult::RouteMatched):
        return std::move(match.handler);
    }
  }

  if (methodNotFound) {
    return getErrorHandler(request, 405);
  } else {
    return getErrorHandler(request, 404);
  }
}

std::function<HTTPResponse()> Router::getErrorHandler(
    std::shared_ptr<const HTTPRequest>& request, int statusCode) {
  auto route = errorRoutes_.find(statusCode);
  if (route == errorRoutes_.end()) {
    return [statusCode]() { return HTTPResponse(statusCode); };
  } else {
    return [request = std::move(request), &handler=route->second]() { return handler(*request); };
  }
}
}
