#pragma once

#include <folly/Conv.h>
#include <folly/Format.h>

namespace sakura {

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
inline T getArgs(int N, const boost::cmatch& matches);

template<>
inline int64_t getArgs<int64_t>(int N, const boost::cmatch& matches) {
  //TODO: Verify overflow/underflow
  //TODO: Probably a way to make the string constexpr
  return folly::to<int64_t>(matches[folly::sformat("__{}", N)].str()); 
}

template<>
inline double getArgs<double>(int N, const boost::cmatch& matches) {
  //TODO: Verify overflow/underflow
  //TODO: Probably a way to make the string constexpr
  return folly::to<double>(matches[folly::sformat("__{}", N)].str()); 
}

template<>
inline std::string getArgs<std::string>(int N, const boost::cmatch& matches) {
  //TODO: Probably a way to make the string constexpr
  return matches[folly::sformat("__{}", N)].str(); 
}

template<>
inline folly::Optional<int64_t> getArgs<folly::Optional<int64_t>>(int N, const boost::cmatch& matches) {
  //TODO: Verify overflow / underflow
  folly::Optional<int64_t> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if(match.matched) {
    ret = folly::to<int64_t>(match.str());
  }
  return ret;
}

template<>
inline folly::Optional<double> getArgs<folly::Optional<double>>(int N, const boost::cmatch& matches) {
  //TODO: Verify overflow / underflow
  folly::Optional<double> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if(match.matched) {
    ret = folly::to<double>(match.str());
  }
  return ret;
}

template<>
inline folly::Optional<std::string> getArgs<folly::Optional<std::string>>(int N, const boost::cmatch& matches) {
  //TODO: Verify overflow / underflow
  folly::Optional<std::string> ret;
  const auto& match = matches[folly::sformat("__{}", N)];
  if(match.matched) {
    ret = match.str();
  }
  return ret;
}

template <int N, typename T>
inline T getArgs(const boost::cmatch& matches) {
  return getArgs<T>(N, matches);
}


template <typename ...ActionArgs, std::size_t... N>
inline HTTPResponse callAction(std::index_sequence<N...>,
                const std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)>& f, 
                const HTTPRequest& request,
                const boost::cmatch& matches) {
  return f(request, getArgs<N, ActionArgs>(matches)...);
}

template <typename ...ActionArgs>
inline HTTPResponse callAction(const std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)>& f,
                const HTTPRequest& request,
                const boost::cmatch& matches) {
  return callAction(std::index_sequence_for<ActionArgs...>{}, f, request, matches);
}

template <typename ...ActionArgs>
Route<ActionArgs...>::Route(
    std::string path,
    std::unordered_set<std::string> methods,
    std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)> action)
    : methods_(std::move(methods)),
      originalPattern_(std::move(path)),
      isStaticRoute_(false) {
  auto regexAndPatternParams = parseRoute(originalPattern_);
  auto functionParams = parseParameters(action);
  const auto& patternParams = regexAndPatternParams.second;
  regex_ = std::move(regexAndPatternParams.first);

  if (patternParams.size() != functionParams.size()) {
      auto error = 
        folly::sformat("Pattern parameter count != function parameter count ({} vs {})\n", 
          patternParams.size(),
          functionParams.size());
      folly::format(&error, "Pattern parameters:\n");
      for(const auto& param: patternParams) {
        folly::format(&error, to_string(param), "\n");
      }
      folly::format(&error, "Function parameters:\n");
      for(const auto& param: functionParams) {
        folly::format(&error, to_string(param), "\n");
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

  action_ = [ action = std::move(action),
              this ](const HTTPRequest& request) {
    boost::cmatch matches;
    const char* path = request.getPath().c_str();
    if (!boost::regex_match(path, matches, regex_)) {
      return RouteMatch::pathNotMatched();
    }
    if (methods_.find(request.getMethod()) == methods_.end()) {
      return RouteMatch::methodNotMatched();
    }

    return RouteMatch::pathMatched( 
        std::function<HTTPResponse(HTTPRequest)>([matches =
                                                       std::move(matches),
                                                  &action](
        const HTTPRequest& request) {
      return callAction(action, request, matches);
    }));
  };
}

}
