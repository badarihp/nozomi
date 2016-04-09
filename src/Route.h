#pragma once

#include <boost/regex.hpp>
#include <string>
#include <vector>

#include "src/HTTPRequest.h"

#include <iostream>
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

std::string to_string(RouteParamType param) {
  switch(param) {
    case(RouteParamType::Int64):
      return "Int64";
    case(RouteParamType::Double):
      return "Double";
    case(RouteParamType::String):
      return "String";
    case(RouteParamType::OptionalInt64):
      return "OptionalInt64";
    case(RouteParamType::OptionalDouble):
      return "OptionalDouble";
    case(RouteParamType::OptionalString):
      return "OptionalString";
  }
}

std::ostream& operator<<(std::ostream& out, RouteParamType param) {
  out << to_string(param);
  return out;
}

std::pair<std::string, std::vector<RouteParamType>> parseRoute(
    const std::string& route);

template<typename T> inline
RouteParamType to_RouteParamType();

template<typename Arg>
void parseParameters(std::vector<RouteParamType>& params) {
  params.push_back(to_RouteParamType<Arg>());
}

template<typename Arg, typename Arg2, typename ...Args>
void parseParameters(std::vector<RouteParamType>& params) {
  params.push_back(to_RouteParamType<Arg>());
  params.push_back(to_RouteParamType<Arg2>());
  parseParameters<Args...>(params);
}

template<typename Ret, typename ...Args>
std::vector<RouteParamType> parseParameters(const std::function<Ret(Args...)>& f) {
  std::vector<RouteParamType> params;
  parseParameters<Args...>(params);
  return params;
}


class BaseRoute {};
class Route : BaseRoute {
  // TODO: This is going to get much more complicated with some cool template
  // magic
  // TODO: Streaming caller
  // TODO: Constructor takes multiple HTTP Methods
 private:
 public:
};



}

#include "src/Route-inl.h"
