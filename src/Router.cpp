#include "src/Router.h"

using std::vector;
using std::unique_ptr;
using std::unordered_map;

namespace nozomi {

Router::Router(unordered_map<int,
                             std::function<folly::Future<HTTPResponse>(
                                 const HTTPRequest&)>> errorRoutes,
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

RouteMatch Router::getHandler(const proxygen::HTTPMessage* request) const {
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
        return match;
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
        return match;
    }
  }

  if (methodNotFound) {
    return RouteMatch(RouteMatchResult::MethodNotMatched, getErrorHandler(405));
  } else {
    return RouteMatch(RouteMatchResult::PathNotMatched, getErrorHandler(404));
  }
}

std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>
Router::getErrorHandler(int statusCode) const {
  auto route = errorRoutes_.find(statusCode);
  if (route == errorRoutes_.end()) {
    return [statusCode](const HTTPRequest&) {
      return HTTPResponse::future(statusCode);
    };
  } else {
    return [&handler = route->second](const HTTPRequest& request) {
      return handler(request);
    };
  }
}
}
