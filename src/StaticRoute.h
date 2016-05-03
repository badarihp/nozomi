#pragma once

#include <string>
#include <unordered_set>

#include "src/BaseRoute.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <proxygen/lib/http/HTTPMethod.h>

namespace sakura {

template <typename HandlerType>
class StaticRoute : public BaseRoute {
  // TODO: Streaming caller
 private:
  HandlerType handler_;

 public:
  StaticRoute(std::string pattern,
              std::unordered_set<proxygen::HTTPMethod> methods,
              HandlerType handler);

  virtual RouteMatch handler(const proxygen::HTTPMessage* request) override;
};

template <typename HandlerType>
inline std::unique_ptr<BaseRoute> make_static_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler) {
  return std::make_unique<StaticRoute<HandlerType>>(
      std::move(pattern), std::move(methods), std::move(handler));
}
}

#include "src/StaticRoute-inl.h"
