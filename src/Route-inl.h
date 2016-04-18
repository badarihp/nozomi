#pragma once

#include <folly/Conv.h>
#include <folly/Format.h>

namespace sakura {
namespace route {

enum class RouteParamType {
  Int64,
  Double,
  String,
  OptionalInt64,
  OptionalDouble,
  OptionalString,
};

std::string to_string(RouteParamType param);
std::ostream& operator<<(std::ostream& out, RouteParamType param);

template <typename T>
inline RouteParamType to_RouteParamType();

template <>
inline RouteParamType to_RouteParamType<int64_t>() {
  return RouteParamType::Int64;
}

template <>
inline RouteParamType to_RouteParamType<folly::Optional<int64_t>>() {
  return RouteParamType::OptionalInt64;
}

template <>
inline RouteParamType to_RouteParamType<double>() {
  return RouteParamType::Double;
}

template <>
inline RouteParamType to_RouteParamType<folly::Optional<double>>() {
  return RouteParamType::OptionalDouble;
}

template <>
inline RouteParamType to_RouteParamType<std::string>() {
  return RouteParamType::String;
}

template <>
inline RouteParamType to_RouteParamType<folly::Optional<std::string>>() {
  return RouteParamType::OptionalString;
}

template <typename T>
inline T get_handler_args(int N,  const boost::smatch& matches);

template <>
inline int64_t get_handler_args<int64_t>(int N,  const boost::smatch& matches) {
  // TODO: There is probably a way to make all of these sformat("__{}", N)
  //        calls constexpr
  try {
    return folly::to<int64_t>(matches[folly::sformat("__{}", N)].str());
  } catch (const std::exception& e) {
    // TODO: Logging
    return std::numeric_limits<int64_t>::max();
  }
}

template <>
inline double get_handler_args<double>(int N,  const boost::smatch& matches) {
  return folly::to<double>(matches[folly::sformat("__{}", N)].str());
}

template <>
inline std::string get_handler_args<std::string>(int N, 
                                                 const boost::smatch& matches) {
  return matches[folly::sformat("__{}", N)].str();
}

template <>
inline folly::Optional<int64_t> get_handler_args<folly::Optional<int64_t>>(
    int N,  const boost::smatch& matches) {
  folly::Optional<int64_t> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if (match.matched) {
    try {
      ret = folly::to<int64_t>(match.str());
    } catch (const std::exception& e) {
      // TODO: Logging
      ret = std::numeric_limits<int64_t>::max();
    }
  }
  return ret;
}

template <>
inline folly::Optional<double> get_handler_args<folly::Optional<double>>(
    int N,  const boost::smatch& matches) {
  folly::Optional<double> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if (match.matched) {
    ret = folly::to<double>(match.str());
  }
  return ret;
}

template <>
inline folly::Optional<std::string>
get_handler_args<folly::Optional<std::string>>(int N, 
                                               const boost::smatch& matches) {
  folly::Optional<std::string> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if (match.matched) {
    ret = match.str();
  }
  return ret;
}

template <int N, typename T>
inline T get_handler_args(const boost::smatch& matches) {
  return get_handler_args<T>(N, matches);
}

template <typename... HandlerArgs, std::size_t... N>
inline HTTPResponse call_handler(
    std::index_sequence<N...>,
    const std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)>& f,
    const HTTPRequest& request,
    const boost::smatch& matches) {
  return f(request, get_handler_args<N, HandlerArgs>(matches)...);
}

template <typename... HandlerArgs>
inline HTTPResponse call_handler(
    const std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)>& f,
    const HTTPRequest& request,
    const boost::smatch& matches) {
  return call_handler(std::index_sequence_for<HandlerArgs...>{}, f, request,
                      matches);
}

template <typename... Args>
inline typename std::enable_if<sizeof...(Args) == 0, void>::type
parse_function_parameters(std::vector<RouteParamType>& params) {}

template <typename Arg1, typename... Args>
inline void parse_function_parameters(std::vector<RouteParamType>& params) {
  params.push_back(to_RouteParamType<Arg1>());
  parse_function_parameters<Args...>(params);
}

template <typename... Args>
std::vector<RouteParamType> parse_function_parameters(
    const std::function<HTTPResponse(const HTTPRequest&, Args...)>& f) {
  std::vector<RouteParamType> params;
  params.reserve(sizeof...(Args));
  parse_function_parameters<Args...>(params);
  return params;
}

std::pair<boost::basic_regex<char>, std::vector<RouteParamType>>
parse_route_pattern(const std::string& route);

}  // end namespace route

template <typename... HandlerArgs>
RouteMatch Route<HandlerArgs...>::handler(std::shared_ptr<const HTTPRequest>& request) {
    //This takes a unique_ptr to a request because the regex matches object
    //has an underlying reference to the path string. An std::move of the original
    //string into the lambda doesn't guarantee that the reference inside of the
    //regex matches is valid afterwards and might lead to UB. Instead, if
    //we match, we move out of the original request into the lambda that we return
    boost::smatch matches;
    const auto& path = request->getPath();
    if (!boost::regex_match(path, matches, regex_)) {
      return RouteMatch(RouteMatchResult::PathNotMatched);
    }
    if (methods_.find(request->getMethod()) == methods_.end()) {
      return RouteMatch(RouteMatchResult::MethodNotMatched);
    }

    return RouteMatch(RouteMatchResult::RouteMatched,
                      std::function<HTTPResponse()>([
                        request = std::move(request),
                        matches = std::move(matches), this
                      ]() {
                        return route::call_handler(handler_, *request, matches);
                      }));
}

template <typename... HandlerArgs>
Route<HandlerArgs...>::Route(
    std::string pattern,
    std::unordered_set<proxygen::HTTPMethod> methods,
    std::function<HTTPResponse(const HTTPRequest&, HandlerArgs...)> handler)
    : BaseRoute(std::move(pattern), std::move(methods), false), handler_(std::move(handler)) {
  auto regexAndPatternParams = route::parse_route_pattern(originalPattern_);
  auto functionParams = route::parse_function_parameters(handler);
  const auto& patternParams = regexAndPatternParams.second;
  regex_ = std::move(regexAndPatternParams.first);

  if (patternParams.size() != functionParams.size()) {
    auto error = folly::sformat(
        "Pattern parameter count != function parameter count ({} vs {})\n",
        patternParams.size(), functionParams.size());
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
          "Pattern parameter {} ({}) does not match function parameter {} ({})",
          i, to_string(patternParams[i]), i, to_string(functionParams[i])));
    }
  }

}
}
