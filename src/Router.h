#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/regex.hpp>

#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Route.h"
#include "src/Util.h"

namespace sakura {

class Router {
 private:
  // list of (route, method -> Route)
  std::list<std::unique_ptr<BaseRoute>> staticRoutes_;
  std::list<std::unique_ptr<BaseRoute>> routes_;
  std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
      errorRoutes_;

 public:
  Router(
      std::vector<std::unique_ptr<BaseRoute>> routes,
      std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
          errorRoutes);
  std::function<HTTPResponse(const HTTPRequest&)> getHandler(
      const HTTPRequest& request);
  std::function<HTTPResponse(const HTTPRequest&)> getErrorHandler(
      int statusCode);
  // TODO: Streaming
  // TODO: Option to automatically append a trailing slash
};

inline void push_routes(std::vector<std::unique_ptr<BaseRoute>>& routes) {}

template <typename Route, typename... Routes>
inline void push_routes(std::vector<std::unique_ptr<BaseRoute>>& routes,
                        Route&& route,
                        Routes&&... remainingRoutes) {
  routes.push_back(std::move(route));
  push_routes(routes, std::move(remainingRoutes)...);
}

template <typename... Routes>
Router make_router(
    std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
        errorRoutes,
    Routes&&... routes) {
  std::vector<std::unique_ptr<BaseRoute>> newRoutes;
  newRoutes.reserve(sizeof...(Routes));
  push_back_move(newRoutes, std::move(routes)...);
  return Router(std::move(newRoutes), std::move(errorRoutes));
}
}
