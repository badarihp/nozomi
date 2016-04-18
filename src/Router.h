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
  std::list<std::unique_ptr<BaseRoute>> staticRoutes_;
  std::list<std::unique_ptr<BaseRoute>> routes_;
  std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
      errorRoutes_;

 public:
  Router(
      std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
          errorRoutes,
      std::vector<std::unique_ptr<BaseRoute>> routes);
  std::function<HTTPResponse()> getHandler(
      std::shared_ptr<const HTTPRequest>& request);
  std::function<HTTPResponse()> getErrorHandler(
      std::shared_ptr<const HTTPRequest>& request, int statusCode);
  // TODO: Streaming
  // TODO: Option to automatically append a trailing slash
};

template <typename... Routes>
Router make_router(
    std::unordered_map<int, std::function<HTTPResponse(const HTTPRequest&)>>
        errorRoutes,
    Routes&&... routes) {
  std::vector<std::unique_ptr<BaseRoute>> newRoutes;
  newRoutes.reserve(sizeof...(Routes));
  push_back_move(newRoutes, std::move(routes)...);
  return Router(std::move(errorRoutes), std::move(newRoutes));
}
}
