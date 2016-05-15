#include "src/RouteParsing.h"

#include <folly/Format.h>
#include <folly/Optional.h>
#include <folly/gen/Base.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include <stdexcept>

using boost::basic_regex;
using folly::Optional;
using folly::sformat;
using proxygen::HTTPMethod;
using std::function;
using std::pair;
using std::runtime_error;
using std::string;
using std::unordered_set;
using std::vector;

namespace nozomi {
namespace route_parsing {

using Replacement = pair<string, string>;

string to_string(RouteParamType param) {
  switch (param) {
    case (RouteParamType::Int64):
      return "Int64";
    case (RouteParamType::Double):
      return "Double";
    case (RouteParamType::String):
      return "String";
    case (RouteParamType::OptionalInt64):
      return "OptionalInt64";
    case (RouteParamType::OptionalDouble):
      return "OptionalDouble";
    case (RouteParamType::OptionalString):
      return "OptionalString";
  }
}

std::ostream& operator<<(std::ostream& out, RouteParamType param) {
  out << to_string(param);
  return out;
}

template <typename T>
Replacement route_to_regex(int paramCount,
                           string originalPattern,
                           const string& value,
                           const string& consumed);
template <>
Replacement route_to_regex<int64_t>(int paramCount,
                                    string originalPattern,
                                    const string& value,
                                    const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+))", paramCount));
}

template <>
Replacement route_to_regex<double>(int paramCount,
                                   string originalPattern,
                                   const string& value,
                                   const string& consumed) {
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>[+-]?\d+(?:\.\d+)?))", paramCount));
}

template <>
Replacement route_to_regex<Optional<int64_t>>(int paramCount,
                                              string originalPattern,
                                              const string& value,
                                              const string& consumed) {
  return std::make_pair(
      std::move(originalPattern),
      sformat(R"((?:(?<__{}>[+-]?\d+){})?)", paramCount, value));
}

template <>
Replacement route_to_regex<Optional<double>>(int paramCount,
                                             string originalPattern,
                                             const string& value,
                                             const string& consumed) {
  return std::make_pair(
      std::move(originalPattern),
      sformat(R"((?:(?<__{}>[+-]?\d+(?:\.\d+)?){})?)", paramCount, value));
}

template <>
Replacement route_to_regex<string>(int paramCount,
                                   string originalPattern,
                                   const string& value,
                                   const string& consumed) {
  // Ensure that the regex provided is valid
  basic_regex<char> re(value, boost::regex::perl);
  return std::make_pair(std::move(originalPattern),
                        sformat(R"((?<__{}>{}))", paramCount, value));
}

template <>
Replacement route_to_regex<Optional<string>>(int paramCount,
                                             string originalPattern,
                                             const string& value,
                                             const string& consumed) {
  // Ensure that the regex provided is valid
  basic_regex<char> re(value, boost::regex::perl);
  return std::make_pair(
      std::move(originalPattern),
      sformat(R"((?:(?<__{}>{}){})?)", paramCount, value, consumed));
}

pair<basic_regex<char>, vector<RouteParamType>> parse_route_pattern(
    const string& pattern) {
  vector<RouteParamType> types;
  string finalRoute = pattern;
  string routeParser =
      R"((?<int>\{\{i\}\})|)"
      R"((?<optional_int>\{\{i\?(?::(?<optional_int_regex>.+?))?\}\})|)"
      R"((?<double>\{\{d\}\})|)"
      R"((?<optional_double>\{\{d\?(?::(?<optional_double_regex>.+?))?\}\})|)"
      R"((?<string>\{\{s:(?<string_regex>.+?)\}\})|)"
      R"((?<optional_string>\{\{s\?:(?<optional_string_regex>.+?)(?::(?<optional_string_consumed>.+?))?\}\}))";

  basic_regex<char> re(routeParser, boost::regex::perl);
  boost::sregex_iterator re_begin(pattern.cbegin(), pattern.cend(), re);
  boost::sregex_iterator re_end;

  // Replace as a second step because iterators don't like you changing the
  // string during iteration
  vector<Replacement> replacements;
  for (auto& match_it = re_begin; match_it != re_end; ++match_it) {
    auto& match = *match_it;
    if (match["int"].matched) {
      replacements.emplace_back(route_to_regex<int64_t>(
          replacements.size(), match["int"].str(), "", ""));
      types.push_back(RouteParamType::Int64);
    } else if (match["optional_int"].matched) {
      replacements.emplace_back(route_to_regex<Optional<int64_t>>(
          replacements.size(), match["optional_int"].str(),
          match["optional_int_regex"].str(), ""));
      types.push_back(RouteParamType::OptionalInt64);
    } else if (match["double"].matched) {
      replacements.emplace_back(route_to_regex<double>(
          replacements.size(), match["double"].str(), "", ""));
      types.push_back(RouteParamType::Double);
    } else if (match["optional_double"].matched) {
      replacements.emplace_back(route_to_regex<Optional<double>>(
          replacements.size(), match["optional_double"].str(),
          match["optional_double_regex"].str(), ""));
      types.push_back(RouteParamType::OptionalDouble);
    } else if (match["string"].matched) {
      replacements.emplace_back(
          route_to_regex<string>(replacements.size(), match["string"].str(),
                                 match["string_regex"].str(), ""));
      types.push_back(RouteParamType::String);
    } else if (match["optional_string"].matched) {
      replacements.emplace_back(route_to_regex<Optional<string>>(
          replacements.size(), match["optional_string"].str(),
          match["optional_string_regex"].str(),
          match["optional_string_consumed"]));
      types.push_back(RouteParamType::OptionalString);
    }
  }

  decltype(finalRoute.find("")) it = 0;
  for (const auto& replacement : replacements) {
    it = finalRoute.find(replacement.first, it);
    if (it == string::npos) {
      auto msg = sformat(
          "Attempted to transform route pattern into regular expression, and"
          " failed. This is a library error, and should be fixed. Tried to"
          " replace {} with {}. Pattern so far: {}.",
          replacement.first, replacement.second, finalRoute);
      throw runtime_error(msg);
    }
    finalRoute.replace(it, replacement.first.size(), replacement.second);
    it += replacement.first.size();
  }
  return std::make_pair(basic_regex<char>(finalRoute, boost::regex::perl),
                        std::move(types));
}
}
}
