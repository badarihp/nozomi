#include "src/Route.h"

using std::string;
using std::pair;
using std::vector;

#include <folly/Format.h>
#include <folly/Optional.h>
#include <iostream>
using folly::sformat;
using folly::Optional;

namespace sakura {

using Replacement = std::pair<std::string, std::string>;

template <typename T>
Replacement routeReplacement(int paramCount,
                             string originalPattern,
                             const string& value);
template <>
Replacement routeReplacement<int>(int paramCount,
                                  string originalPattern,
                                  const string& value) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+))", paramCount));
}

template <>
Replacement routeReplacement<double>(int paramCount,
                                     string originalPattern,
                                     const string& value) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]\d+(?:\.\d+)))", paramCount));
}

template <>
Replacement routeReplacement<Optional<int>>(int paramCount,
                                            string originalPattern,
                                            const string& value) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+)?)", paramCount));
}

template <>
Replacement routeReplacement<Optional<double>>(int paramCount,
                                               string originalPattern,
                                               const string& value) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]\d+(?:\.\d+))?)", paramCount));
}

template <>
Replacement routeReplacement<string>(int paramCount,
                                     string originalPattern,
                                     const string& value) {
  // Ensure that the regex provided is valid
  boost::regex re(value);
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>{}))", paramCount, value));
}

template <>
Replacement routeReplacement<Optional<string>>(int paramCount,
                                               string originalPattern,
                                               const string& value) {
  // Ensure that the regex provided is valid
  boost::regex re(value);
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>{})?)", paramCount, value));
}

pair<string, vector<RouteParamType>> parseRoute(const string& route) {
  vector<RouteParamType> types;
  string finalRoute = route;
  string routeParser =
      R"((?<int>\{\{i\}\})|)"
      R"((?<double>\{\{d\}\})|)"
      R"((?<optional_int>\{\{i\?\}\})|)"
      R"((?<optional_double>\{\{d\?\}\})|)"
      R"((?<string>\{\{s:(?<string_regex>.+?)\}\})|)"
      R"((?<optional_string>\{\{s:(<optional_string_regex>.+?)\}\}))";

  boost::basic_regex<char> re(routeParser, boost::regex::perl);
  boost::sregex_iterator re_begin(route.cbegin(), route.cend(), re);
  boost::sregex_iterator re_end;

  //Replace as a second step because iterators don't like you changing the string during iteration
  vector<Replacement> replacements; 
  for (auto& match_it = re_begin; match_it != re_end; ++match_it) {
    auto& match = *match_it;
    if (match["int"].matched) {
      replacements.emplace_back(
          routeReplacement<int>(replacements.size(), match["int"].str(), ""));
      types.push_back(RouteParamType::Int64);
    } else if (match["optional_int"].matched) {
      replacements.emplace_back(routeReplacement<Optional<int>>(
          replacements.size(), match["optional_int"].str(), ""));
      types.push_back(RouteParamType::OptionalInt64);
    } else if (match["double"].matched) {
      replacements.emplace_back(routeReplacement<double>(
          replacements.size(), match["double"].str(), ""));
      types.push_back(RouteParamType::Double);
    } else if (match["optional_double"].matched) {
      replacements.emplace_back(routeReplacement<Optional<double>>(
          replacements.size(), match["optional_double"].str(), ""));
      types.push_back(RouteParamType::OptionalDouble);
    } else if (match["string"].matched) {
      replacements.emplace_back(
          routeReplacement<string>(replacements.size(), match["string"].str(),
                                   match["string_regex"].str()));
      types.push_back(RouteParamType::String);
    } else if (match["optional_string"].matched) {
      replacements.emplace_back(routeReplacement<Optional<string>>(
          replacements.size(), match["optional_string"].str(),
          match["optional_string_regex"].str()));
      types.push_back(RouteParamType::OptionalString);
    }
  }

  for (const auto& replacement : replacements) {
    auto it = finalRoute.find(replacement.first);
    if(it != string::npos) {
      finalRoute.replace(it, replacement.first.size(), replacement.second);
    }
  }

  return std::make_pair(std::move(finalRoute), std::move(types));
}

}
