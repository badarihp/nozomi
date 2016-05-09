#pragma once
namespace sakura {

namespace {
template <typename HandlerType, bool IsStreaming>
struct RouteMatchMaker {};

template <typename HandlerType>
struct RouteMatchMaker<HandlerType, false> {
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
struct RouteMatchMaker<HandlerType, true> {
  inline RouteMatch operator()(HandlerType& handler) {
    return RouteMatch(
        RouteMatchResult::RouteMatched,
        std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>(),
        std::function<proxygen::RequestHandler*()>([&handler]() mutable {
          auto* ret = handler();
          ret->setRequestArgs();
          return ret;
        }));
  }
};
}

template <typename HandlerType>
StaticRoute<HandlerType>::StaticRoute(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler)
    : BaseRoute(std::move(pattern), std::move(methods), true),
      handler_(std::move(handler)) {}

template <typename HandlerType>
RouteMatch StaticRoute<HandlerType>::handler(
    const proxygen::HTTPMessage* request) {
  DCHECK(request != nullptr);
  auto methodAndPath = HTTPRequest::getMethodAndPath(request);
  if (std::get<1>(methodAndPath) != originalPattern_) {
    return RouteMatch(RouteMatchResult::PathNotMatched);
  }
  if (methods_.find(std::get<0>(methodAndPath)) == methods_.end()) {
    return RouteMatch(RouteMatchResult::MethodNotMatched);
  }
  return RouteMatchMaker<
      HandlerType,

      std::is_convertible<decltype(std::declval<HandlerType>()()),
                          StreamingHTTPHandler<>*>::value>{}(handler_);
}
}
