#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/regex.hpp>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/BaseRoute.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/Util.h"

namespace nozomi {

class Router {
 private:
  std::list<std::unique_ptr<BaseRoute>> staticRoutes_;
  std::list<std::unique_ptr<BaseRoute>> routes_;
  std::unordered_map<
      int,
      std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>>
      errorRoutes_;

 public:
  Router(std::unordered_map<
             int,
             std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>>
             errorRoutes,
         std::vector<std::unique_ptr<BaseRoute>> routes);
  //  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>
  RouteMatch getHandler(const proxygen::HTTPMessage* request) const;
  std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>
  getErrorHandler(int statusCode) const;
  // TODO: Streaming
  // TODO: Option to automatically append a trailing slash
};

template <typename... Routes>
Router make_router(std::unordered_map<int,
                                      std::function<folly::Future<HTTPResponse>(
                                          const HTTPRequest&)>> errorRoutes,
                   std::unique_ptr<Routes>... routes) {
  std::vector<std::unique_ptr<BaseRoute>> newRoutes;
  newRoutes.reserve(sizeof...(Routes));
  push_back_move(newRoutes, std::move(routes)...);
  return Router(std::move(errorRoutes), std::move(newRoutes));
}

template <typename... Routes>
Router make_router(std::unordered_map<int,
                                      std::function<folly::Future<HTTPResponse>(
                                          const HTTPRequest&)>> errorRoutes,
                   Routes&&... routes) {
  std::vector<std::unique_ptr<BaseRoute>> newRoutes;
  newRoutes.reserve(sizeof...(Routes));
  push_back_move(newRoutes, std::move(routes)...);
  return Router(std::move(errorRoutes), std::move(newRoutes));
}
}
