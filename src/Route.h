#pragma once

#include <boost/regex.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/BaseRoute.h"
#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <folly/Optional.h>
#include <proxygen/lib/http/HTTPMethod.h>

namespace sakura {

template <typename... HandlerArgs>
class Route : public BaseRoute {
  // TODO: Streaming caller
 private:
  std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler_;
  boost::basic_regex<char> regex_;

 public:
  Route(
      std::string pattern,
      std::unordered_set<proxygen::HTTPMethod> methods,
      std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler);
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) const;
};

template <typename... HandlerArgs>
inline std::unique_ptr<Route<HandlerArgs...>> make_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler) {
  return std::make_unique<Route<HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

}

#include "src/Route-inl.h"
