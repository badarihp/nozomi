#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "src/BaseRoute.h"
#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"
#include "src/StreamingHTTPHandler.h"
#include "src/Util.h"

#include <boost/regex.hpp>
#include <folly/Optional.h>
#include <folly/futures/Future.h>
#include <proxygen/lib/http/HTTPMethod.h>

namespace nozomi {

template <typename HandlerType, typename... HandlerArgs>
class Route : public BaseRoute {
  // TODO: Streaming caller
 private:
  HandlerType handler_;
  boost::basic_regex<char> regex_;

 public:
  Route(std::string pattern,
        std::unordered_set<proxygen::HTTPMethod> methods,
        HandlerType handler);
  virtual RouteMatch handler(const proxygen::HTTPMessage* request) override;
};

template <typename... HandlerArgs>
inline auto make_route(std::string pattern,
                       std::unordered_set<proxygen::HTTPMethod> methods,
                       folly::Future<HTTPResponse> (*handler)(
                           const HTTPRequest&, HandlerArgs...)) {
  return std::make_unique<Route<decltype(handler), HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

template <typename RequestHandlerType>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    RequestHandlerType* (*handler)()) {
  // TODO: Tests
  return make_streaming_route<decltype(handler)>(
      std::move(pattern), std::move(methods), std::move(handler));
}

template <typename HandlerType, typename... HandlerArgs>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler,
    type_sequence<HandlerArgs...>) {
  return std::make_unique<Route<HandlerType, HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

template <typename HandlerType>
inline auto make_streaming_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler) {
  // TODO: Tests to make sure that we don't break inferring
  //       handler variables

  // Get the type that comes out of the RequestHandler factory
  using RequestHandler =
      typename std::remove_pointer<decltype(handler())>::type;
  // Using that type, get a pointer to the setRequestArgs method so
  // that we can rip the HandlerArgs... types out without having
  // to set them explicitly
  using CallablePointer = decltype(&RequestHandler::setRequestArgs);
  auto types = make_type_sequence<CallablePointer>();
  return make_streaming_route(std::move(pattern), std::move(methods),
                              std::move(handler), types);
}

template <typename HandlerType, typename Request, typename... HandlerArgs>
inline std::unique_ptr<Route<HandlerType, HandlerArgs...>> make_route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler,
    type_sequence<Request, HandlerArgs...>) {
  return std::make_unique<Route<HandlerType, HandlerArgs...>>(
      std::move(pattern), std::move(methods), std::move(handler));
}

template <typename HandlerType>
inline auto make_route(std::string pattern,
                       std::unordered_set<proxygen::HTTPMethod> methods,
                       HandlerType handler) {
  auto types = make_type_sequence(handler);
  return make_route(std::move(pattern), std::move(methods), std::move(handler),
                    types);
}
}

#include "src/Route-inl.h"
