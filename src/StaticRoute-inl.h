#pragma once
namespace sakura {

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
  return RouteMatch(
      RouteMatchResult::RouteMatched,
      std::function<HTTPResponse(const HTTPRequest&)>([this](
          const HTTPRequest& request) mutable { return handler_(request); }));
}
}
