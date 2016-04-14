#pragma once

#include <boost/regex.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"
#include "src/HTTPResponse.h"

#include <folly/Optional.h>

namespace sakura {

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

std::pair<boost::basic_regex<char>, std::vector<RouteParamType>> parseRoute(
    const std::string& route);

// Get the mapping from int, string, etc to RouteParamType
template <typename T>
inline RouteParamType to_RouteParamType();

template <typename... Args>
inline typename std::enable_if<sizeof...(Args) == 0, void>::type
parseParameters(std::vector<RouteParamType>& params) {}

template <typename Arg1, typename... Args>
inline void parseParameters(std::vector<RouteParamType>& params) {
  params.push_back(to_RouteParamType<Arg1>());
  parseParameters<Args...>(params);
}

template <typename... Args>
std::vector<RouteParamType> parseParameters(
    const std::function<HTTPResponse(const HTTPRequest&, Args...)>& f) {
  std::vector<RouteParamType> params;
  params.reserve(sizeof...(Args));
  parseParameters<Args...>(params);
  return params;
}

struct RouteMatch {
 private:
  RouteMatch(bool matchedPath,
             bool methodAllowed,
             std::function<HTTPResponse(const HTTPRequest&)> action)
      : matchedPath(matchedPath),
        methodAllowed(methodAllowed),
        action(std::move(action)) {}

 public:
  const bool matchedPath;
  const bool methodAllowed;
  const std::function<HTTPResponse(const HTTPRequest&)> action;
  inline static RouteMatch pathNotMatched() {
    return RouteMatch(false, false,
                      std::function<HTTPResponse(const HTTPRequest&)>());
  }
  inline static RouteMatch methodNotMatched() {
    return RouteMatch(true, false,
                      std::function<HTTPResponse(const HTTPRequest&)>());
  }
  inline static RouteMatch pathMatched(
      std::function<HTTPResponse(const HTTPRequest&)> f) {
    return RouteMatch(true, true, std::move(f));
  }
};

class BaseRoute {
 protected:
  std::function<RouteMatch(const HTTPRequest&)> action_;

 public:
  RouteMatch action(const HTTPRequest& request);
};

template <typename... ActionArgs>
class Route : public virtual BaseRoute {
  // TODO: This is going to get much more complicated with some cool template
  // magic
  // TODO: Streaming caller
  // TODO: Constructor takes multiple HTTP Methods
 private:
  std::unordered_set<std::string> methods_;
  std::string originalPattern_;
  bool isStaticRoute_;
  boost::basic_regex<char> regex_;

 public:
  // Route(std::string path, std::unordered_set<std::string> methods,
  // std::function<HTTPResponse(HTTPRequest)> action, bool isStatic = true);
  Route(std::string path,
        std::unordered_set<std::string> methods,
        std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)> action);
};

template <typename... ActionArgs>
inline Route<ActionArgs...> make_route(
    std::string path,
    std::unordered_set<std::string> methods,
    std::function<HTTPResponse(const HTTPRequest&, ActionArgs...)> action) {
  return Route<ActionArgs...>(std::move(path), std::move(methods),
                              std::move(action));
}
}

#include "src/Route-inl.h"
