#include "src/Router.h"

using std::vector;
using std::unique_ptr;
using std::unordered_map;

namespace sakura {

    Router::Router(
      vector<unique_ptr<BaseRoute>> routes,
      unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>> errorRoutes):
        errorRoutes_(std::move(errorRoutes)) {
      for(auto& route: routes) {
        if(route->isStaticRoute()) {
          staticRoutes_.push_back(std::move(route));
        } else {
          routes_.push_back(std::move(route));
        }
      }
    }

    std::function<HTTPResponse(const HTTPRequest&)> Router::getHandler(const HTTPRequest& request) {
      // Check static routes first, then dynamic ones
      bool methodNotFound = false;
      for(const auto& route: staticRoutes_) {
        auto match = route->handler(request);
        switch(match.result) {
          case(RouteMatchResult::PathNotMatched):
            continue;
          case(RouteMatchResult::MethodNotMatched):
            methodNotFound = true;
            break;
          case(RouteMatchResult::RouteMatched):
            return std::move(match.handler);
        }
      }

      for(const auto& route: routes_) {
        auto match = route->handler(request);
        switch(match.result) {
          case(RouteMatchResult::PathNotMatched):
            continue;
          case(RouteMatchResult::MethodNotMatched):
            methodNotFound = true;
            break;
          case(RouteMatchResult::RouteMatched):
            return std::move(match.handler);
        }
      }

      if(methodNotFound) {
        return getErrorHandler(405);
      } else {
        return getErrorHandler(404);
      }

    }

    std::function<HTTPResponse(const HTTPRequest&)> Router::getErrorHandler(int statusCode)
    {
      auto route = errorRoutes_.find(statusCode);
      if(route == errorRoutes_.end()) {
        return [statusCode](const HTTPRequest& request) { return HTTPResponse(statusCode); };
      } else {
        return route->second;
      }
    }

}

