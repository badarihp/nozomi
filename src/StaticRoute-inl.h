#pragma once
namespace nozomi {

namespace {
template <typename HandlerType, bool IsStreaming>
struct StaticRouteMatchMaker {};

template <typename HandlerType>
struct StaticRouteMatchMaker<HandlerType, false> {
  inline RouteMatch operator()(HandlerType& handler) {
    return RouteMatch(
        RouteMatchResult::RouteMatched,
        std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>(
            [&handler](const HTTPRequest& request) mutable {
              return handler(request);
            }));
  }
};

template <typename HandlerType>
struct StaticRouteMatchMaker<HandlerType, true> {
  inline RouteMatch operator()(HandlerType& handler) {
    return RouteMatch(
        RouteMatchResult::RouteMatched,
        std::function<proxygen::RequestHandler*()>([&handler]() mutable {
          decltype(handler()) ret = nullptr;
          try {
            ret = handler();
            if (ret == nullptr) {
              return ret;
            }
            ret->setRequestArgs();
          } catch (const std::exception& e) {
            // TODO: Logging
            if (ret != nullptr) {
              delete ret;
            }
            ret = nullptr;
          }
          return ret;

        }));
  }
};
}

template <typename HandlerType, bool IsStreaming>
StaticRoute<HandlerType, IsStreaming>::StaticRoute(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler)
    : BaseRoute(std::move(pattern), std::move(methods), true),
      handler_(std::move(handler)) {}

template <typename HandlerType, bool IsStreaming>
RouteMatch StaticRoute<HandlerType, IsStreaming>::handler(
    const proxygen::HTTPMessage* request) {
  DCHECK(request != nullptr);
  auto methodAndPath = HTTPRequest::getMethodAndPath(request);
  if (std::get<1>(methodAndPath) != originalPattern_) {
    return RouteMatch(RouteMatchResult::PathNotMatched);
  }
  if (methods_.find(std::get<0>(methodAndPath)) == methods_.end()) {
    return RouteMatch(RouteMatchResult::MethodNotMatched);
  }
  return StaticRouteMatchMaker<HandlerType, IsStreaming>{}(handler_);
}
}
