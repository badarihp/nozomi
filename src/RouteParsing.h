#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/regex.hpp>
#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/Optional.h>

namespace nozomi {
namespace route_parsing {

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
inline T get_handler_args(int N, const boost::smatch& matches);

template <>
inline int64_t get_handler_args<int64_t>(int N, const boost::smatch& matches) {
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
inline double get_handler_args<double>(int N, const boost::smatch& matches) {
  return folly::to<double>(matches[folly::sformat("__{}", N)].str());
}

template <>
inline std::string get_handler_args<std::string>(int N,
                                                 const boost::smatch& matches) {
  return matches[folly::sformat("__{}", N)].str();
}

template <>
inline folly::Optional<int64_t> get_handler_args<folly::Optional<int64_t>>(
    int N, const boost::smatch& matches) {
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
    int N, const boost::smatch& matches) {
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

template <typename... Args>
inline typename std::enable_if<sizeof...(Args) == 0, void>::type
parse_function_parameters(std::vector<RouteParamType>& params) {}

template <typename Arg1, typename... Args>
inline void parse_function_parameters(std::vector<RouteParamType>& params) {
  params.push_back(to_RouteParamType<Arg1>());
  parse_function_parameters<Args...>(params);
}

template <typename... Args>
std::vector<RouteParamType> parse_function_parameters() {
  std::vector<RouteParamType> params;
  params.reserve(sizeof...(Args));
  parse_function_parameters<Args...>(params);
  return params;
}

std::pair<boost::basic_regex<char>, std::vector<RouteParamType>>
parse_route_pattern(const std::string& route);
}
}
