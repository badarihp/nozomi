#pragma once

#include <string>
#include <unordered_set>

#include "src/BaseRoute.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <proxygen/lib/http/HTTPMethod.h>

namespace sakura {

class StaticRoute : public BaseRoute {
  // TODO: Streaming caller
 private:
  std::function<HTTPResponse(const HTTPRequest&)> handler_;

 public:
  StaticRoute(std::string pattern,
              std::unordered_set<proxygen::HTTPMethod> methods,
              std::function<HTTPResponse(const HTTPRequest& request)> handler);

  virtual RouteMatch handler(const proxygen::HTTPMessage* request) const;
};

inline std::unique_ptr<BaseRoute> make_static_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    std::function<HTTPResponse(const HTTPRequest& request)> handler) {
  return std::make_unique<StaticRoute>(std::move(pattern), std::move(methods),
                                       std::move(handler));
}
}

#include "src/StaticRoute-inl.h"
