#include "src/Route.h"

using std::string;
using std::pair;
using std::vector;
using boost::basic_regex;

#include <folly/Format.h>
#include <folly/Optional.h>
#include <iostream>
using folly::sformat;
using folly::Optional;

namespace sakura {

using Replacement = std::pair<std::string, std::string>;

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

template <typename T>
Replacement routeReplacement(int paramCount,
                             string originalPattern,
                             const string&value, const string& consumed);
template <>
Replacement routeReplacement<int64_t>(int paramCount,
                                  string originalPattern,
                                  const string&value, const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+))", paramCount));
}

template <>
Replacement routeReplacement<double>(int paramCount,
                                     string originalPattern,
                                     const string&value, const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+(?:\.\d+)?))", paramCount));
}

template <>
Replacement routeReplacement<Optional<int64_t>>(int paramCount,
                                            string originalPattern,
                                            const string&value, const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?:(?<__{}>[+-]?\d+){})?)", paramCount, value));
}

template <>
Replacement routeReplacement<Optional<double>>(int paramCount,
                                               string originalPattern,
                                               const string&value, const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?:(?<__{}>[+-]?\d+(?:\.\d+)?){})?)", paramCount, value));
}

template <>
Replacement routeReplacement<string>(int paramCount,
                                     string originalPattern,
                                     const string&value, const string& consumed) {
  // Ensure that the regex provided is valid
  basic_regex<char> re(value, boost::regex::perl);
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>{}))", paramCount, value));
}

template <>
Replacement routeReplacement<Optional<string>>(int paramCount,
                                               string originalPattern,
                                               const string&value, const string& consumed) {
  // Ensure that the regex provided is valid
  basic_regex<char> re(value, boost::regex::perl);
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?:(?<__{}>{}){})?)", paramCount, value, consumed));
}

pair<basic_regex<char>, vector<RouteParamType>> parseRoute(const string& route) {
  vector<RouteParamType> types;
  string finalRoute = route;
  string routeParser =
      R"((?<int>\{\{i(?::(?<int_regex>.+?))?\}\})|)"
      R"((?<optional_int>\{\{i\?(?::(?<optional_int_regex>.+?))?\}\})|)"
      R"((?<double>\{\{d(?::(?<double_regex>.+?))?\}\})|)"
      R"((?<optional_double>\{\{d\?(?::(?<optional_double_regex>.+?))?\}\})|)"
      R"((?<string>\{\{s:(?<string_regex>.+?)\}\})|)"
      R"((?<optional_string>\{\{s\?:(?<optional_string_regex>.+?)(?::(?<optional_string_consumed>.+?))?\}\}))";

  basic_regex<char> re(routeParser, boost::regex::perl);
  boost::sregex_iterator re_begin(route.cbegin(), route.cend(), re);
  boost::sregex_iterator re_end;

  //Replace as a second step because iterators don't like you changing the string during iteration
  vector<Replacement> replacements; 
  for (auto& match_it = re_begin; match_it != re_end; ++match_it) {
    auto& match = *match_it;
    if (match["int"].matched) {
      replacements.emplace_back(
          routeReplacement<int64_t>(replacements.size(), match["int"].str(), match["int_regex"].str(), ""));
      types.push_back(RouteParamType::Int64);
    } else if (match["optional_int"].matched) {
      replacements.emplace_back(routeReplacement<Optional<int64_t>>(
          replacements.size(), match["optional_int"].str(), match["optional_int_regex"].str(), ""));
      types.push_back(RouteParamType::OptionalInt64);
    } else if (match["double"].matched) {
      replacements.emplace_back(routeReplacement<double>(
          replacements.size(), match["double"].str(), match["double_regex"].str(), ""));
      types.push_back(RouteParamType::Double);
    } else if (match["optional_double"].matched) {
      replacements.emplace_back(routeReplacement<Optional<double>>(
          replacements.size(), match["optional_double"].str(), match["optional_double_regex"].str(), ""));
      types.push_back(RouteParamType::OptionalDouble);
    } else if (match["string"].matched) {
      replacements.emplace_back(
          routeReplacement<string>(replacements.size(), match["string"].str(),
                                   match["string_regex"].str(), ""));
      types.push_back(RouteParamType::String);
    } else if (match["optional_string"].matched) {
      replacements.emplace_back(routeReplacement<Optional<string>>(
          replacements.size(), match["optional_string"].str(),
          match["optional_string_regex"].str(),
          match["optional_string_consumed"]));
      types.push_back(RouteParamType::OptionalString);
    }
  }

  for (const auto& replacement : replacements) {
    auto it = finalRoute.find(replacement.first);
    if(it != string::npos) {
      finalRoute.replace(it, replacement.first.size(), replacement.second);
    }
  }
  std::cout << finalRoute << std::endl;
  return std::make_pair(basic_regex<char>(finalRoute, boost::regex::perl), std::move(types));
}

RouteMatch BaseRoute::action(const HTTPRequest& request) {
  return action_(request);
}

}
