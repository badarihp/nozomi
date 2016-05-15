#pragma once
#include <folly/Conv.h>
#include <folly/Format.h>

#include "src/RouteParsing.h"

namespace nozomi {
namespace {

template <typename... HandlerArgs, std::size_t... N>
inline void call_streaming_handler(
    std::index_sequence<N...>,
    type_sequence<HandlerArgs...>,
    StreamingHTTPHandler<HandlerArgs...>& handler,
    const boost::smatch& matches) {
  return handler.setRequestArgs(
      route_parsing::get_handler_args<N, HandlerArgs>(matches)...);
}

template <typename... HandlerArgs>
inline void call_streaming_handler(
    StreamingHTTPHandler<HandlerArgs...>& handler,
    const boost::smatch& matches) {
  return call_streaming_handler(std::index_sequence_for<HandlerArgs...>{},
                                type_sequence<HandlerArgs...>{}, handler,
                                matches);
}

template <typename HandlerType, typename... HandlerArgs, std::size_t... N>
inline folly::Future<HTTPResponse> call_handler(std::index_sequence<N...>,
                                                type_sequence<HandlerArgs...>,
                                                HandlerType& f,
                                                const HTTPRequest& request,
                                                const boost::smatch& matches) {
  return f(request,
           route_parsing::get_handler_args<N, HandlerArgs>(matches)...);
}

template <typename HandlerType, typename... HandlerArgs>
inline folly::Future<HTTPResponse> call_handler(HandlerType& f,
                                                const HTTPRequest& request,
                                                const boost::smatch& matches) {
  return call_handler(std::index_sequence_for<HandlerArgs...>{},
                      type_sequence<HandlerArgs...>{}, f, request, matches);
}

template <typename HandlerType, bool IsStreaming, typename... HandlerArgs>
struct RegexRouteMatchMaker {};

template <typename HandlerType, typename... HandlerArgs>
struct RegexRouteMatchMaker<HandlerType, false, HandlerArgs...> {
  inline RouteMatch operator()(std::shared_ptr<const std::string>& path,
                               boost::smatch& matches,
                               HandlerType& handler) {
    return RouteMatch(
        RouteMatchResult::RouteMatched,
        std::function<folly::Future<HTTPResponse>(const HTTPRequest&)>([
          path = std::move(path), matches = std::move(matches), &handler
        ](const HTTPRequest& request) mutable {
          // We have to hold onto the original path variable because
          // the matches object
          // retains a reference to the string that was matched on.
          // We use a unique_ptr
          // because there's no absolute guarantee that an std::move
          // won't invalidate the
          // reference to the original string.
          return call_handler<HandlerType, HandlerArgs...>(handler, request,
                                                           matches);
        }));
  }
};

template <typename HandlerType, typename... HandlerArgs>
struct RegexRouteMatchMaker<HandlerType, true, HandlerArgs...> {
  inline RouteMatch operator()(std::shared_ptr<const std::string>& path,
                               boost::smatch& matches,
                               HandlerType& handler) {
    // TODO: Fill this out properly
    return RouteMatch(
        RouteMatchResult::RouteMatched,
        std::function<proxygen::RequestHandler*()>([
          path = std::move(path), matches = std::move(matches), &handler
        ]() {
          // TODO: Exception handling to cleanup memory
          decltype(handler()) ret = nullptr;
          try {
            auto ret = handler();
            if (ret == nullptr) {
              return ret;
            }
            call_streaming_handler<HandlerArgs...>(*ret, matches);
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

}  // end namespace route

template <typename HandlerType, bool IsStreaming, typename... HandlerArgs>
RouteMatch Route<HandlerType, IsStreaming, HandlerArgs...>::handler(
    const proxygen::HTTPMessage* request) {
  DCHECK(request != nullptr) << "Request must not be null";
  boost::smatch matches;
  auto methodAndPath = HTTPRequest::getMethodAndPath(request);
  // TODO: Make this unique when using folly::function
  auto path = std::make_shared<const std::string>(
      std::move(std::get<1>(methodAndPath)));
  if (!boost::regex_match(*path, matches, regex_)) {
    return RouteMatch(RouteMatchResult::PathNotMatched);
  }
  if (methods_.find(std::get<0>(methodAndPath)) == methods_.end()) {
    return RouteMatch(RouteMatchResult::MethodNotMatched);
  }

  return RegexRouteMatchMaker<HandlerType, IsStreaming, HandlerArgs...>{}(
      path, matches, handler_);
}

template <typename HandlerType, bool IsStreaming, typename... HandlerArgs>
Route<HandlerType, IsStreaming, HandlerArgs...>::Route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    HandlerType handler)
    : BaseRoute(std::move(pattern), std::move(methods), false),
      handler_(std::move(handler)) {
  auto regexAndPatternParams =
      route_parsing::parse_route_pattern(originalPattern_);
  auto functionParams =
      route_parsing::parse_function_parameters<HandlerArgs...>();
  const auto& patternParams = regexAndPatternParams.second;
  regex_ = std::move(regexAndPatternParams.first);

  if (patternParams.size() != functionParams.size()) {
    auto error = folly::sformat(
        "Pattern parameter count != function parameter count ({} vs {})\n",
        patternParams.size(), functionParams.size());
    folly::format(&error, "Pattern: {}\n", originalPattern_);
    folly::format(&error, "Pattern parameters:\n");
    for (const auto& param : patternParams) {
      folly::format(&error, "{}\n", to_string(param));
    }
    folly::format(&error, "Function parameters:\n");
    for (const auto& param : functionParams) {
      folly::format(&error, "{}\n", to_string(param));
    }
    throw std::runtime_error(error);
  }

  for (size_t i = 0; i < patternParams.size(); ++i) {
    if (patternParams[i] != functionParams[i]) {
      throw std::runtime_error(folly::sformat(
          "Pattern: {}\n"
          "Pattern parameter {} ({}) does not match function parameter {} ({})",
          originalPattern_, i, to_string(patternParams[i]), i,
          to_string(functionParams[i])));
    }
  }
}
}
