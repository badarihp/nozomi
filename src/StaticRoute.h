#pragma once

#include <string>
#include <unordered_set>

#include "src/BaseRoute.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <proxygen/lib/http/HTTPMethod.h>

namespace nozomi {

template <typename HandlerType, bool IsStreaming>
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
  return std::make_unique<StaticRoute<HandlerType, false>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

template <typename HandlerType>
inline std::unique_ptr<BaseRoute> make_streaming_static_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler) {
  return std::make_unique<StaticRoute<HandlerType, true>>(
      std::move(pattern), std::move(methods), std::move(handler));
}
}

#include "src/StaticRoute-inl.h"
