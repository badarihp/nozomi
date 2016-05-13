#include "src/PostParser.h"

#include <utility>

#include <folly/String.h>
#include <folly/gen/Base.h>
#include <folly/gen/String.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>

#include "src/StringUtils.h"

using folly::IOBuf;
using folly::Optional;
using folly::StringPiece;
using std::unordered_map;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;
using proxygen::HTTPHeaderCode;
using namespace folly::gen;

namespace sakura {
PostParser::PostParser(const HTTPRequest& request)
    : originalBody_(request.getBodyAsBytes()),
      parsedData_(parseRequest(request, originalBody_)) {}

unordered_map<string, vector<unique_ptr<IOBuf>>> PostParser::parseRequest(
    const HTTPRequest& request, const unique_ptr<IOBuf>& body) {
  auto contentType =
      request.getHeaders()[HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE];
  if (!contentType) {
    return unordered_map<string, vector<unique_ptr<IOBuf>>>();
  }
  if (*contentType == "application/x-www-form-urlencoded") {
    return parseUrlEncoded(body);
  }
  if (*contentType == "multipart/form-data") {
    return parseFormData(body);
  }
  return unordered_map<string, vector<unique_ptr<IOBuf>>>();
}

Optional<string> PostParser::getString(const string& key) {
  auto it = parsedData_.find(key);
  if (it == parsedData_.end()) {
    return Optional<string>();
  } else {
    return Optional<string>(to_string(it->second.front()));
  }
}

Optional<vector<string>> PostParser::getStringList(const string& key) {
  auto it = parsedData_.find(key);
  if (it == parsedData_.end()) {
    return Optional<vector<string>>();
  } else {
    return Optional<vector<string>>(
        from(it->second) | map([](const auto& buf) { return to_string(buf); }) |
        as<vector>());
  };
}

Optional<unique_ptr<IOBuf>> PostParser::getBinary(const string& key) {
  auto it = parsedData_.find(key);
  if (it == parsedData_.end()) {
    return Optional<unique_ptr<IOBuf>>();
  } else {
    return Optional<unique_ptr<IOBuf>>(it->second.front()->clone());
  };
}

Optional<vector<unique_ptr<IOBuf>>> PostParser::getBinaryList(
    const string& key) {
  auto it = parsedData_.find(key);
  if (it == parsedData_.end()) {
    return Optional<vector<unique_ptr<IOBuf>>>();
  } else {
    return Optional<vector<unique_ptr<IOBuf>>>(
        from(it->second) | map([](const auto& buf) { return buf->clone(); }) |
        move | as<vector<unique_ptr<IOBuf>>>());
  };
}

unordered_map<string, vector<unique_ptr<IOBuf>>> PostParser::parseUrlEncoded(
    const unique_ptr<IOBuf>& body) {
  // TODO: This completely ignores encoding headers when parsing. If you send it
  //      binary data, it's going to try its damndest to split and give you
  //      binary
  //      data back, which will later get stuffed into a string. meh for now,
  //      but
  //      be careful using .data()/.c_str() on things that expect a raw char*
  //      with
  //      no length delimiter
  // TODO: Optimize this to get rid of the extra copies. Chained IOBuf
  // ownership: It's complicated
  unordered_map<string, vector<unique_ptr<IOBuf>>> ret;
  auto asString = to_string(body);
  auto kvps =
      split(asString, "&") | filter([](StringPiece& keyValueString) {
        return keyValueString.size() != 0;
      }) |
      map([](StringPiece keyValueString) {
        folly::StringPiece key, value;
        if (!folly::split<false>("=", keyValueString, key, value)) {
          key = keyValueString;
        }
        return std::make_pair<StringPiece, StringPiece>(std::move(key),
                                                        std::move(value));
      }) |
      map([](pair<StringPiece, StringPiece> kvp) {
        // Ignore conversion failures. Better to give /something/ rather than
        // nothing.
        string key, value;
        try {
          folly::uriUnescape(kvp.first, key, folly::UriEscapeMode::QUERY);
        } catch (const std::exception& e) {
          key = kvp.first.toString();
        }
        try {
          folly::uriUnescape(kvp.second, value, folly::UriEscapeMode::QUERY);
        } catch (const std::exception& e) {
          value = kvp.second.toString();
        }
        return std::make_pair<string, string>(std::move(key), std::move(value));
      }) |
      as<vector>();
  ret.reserve(kvps.size());
  for (auto& kvp : kvps) {
    ret[std::move(kvp.first)].push_back(
        IOBuf::copyBuffer(std::move(kvp.second)));
  }
  return ret;
}

unordered_map<string, vector<unique_ptr<IOBuf>>> PostParser::parseFormData(
    const unique_ptr<IOBuf>& body) {
  // TODO: https://tools.ietf.org/html/rfc7578
  return unordered_map<string, vector<unique_ptr<IOBuf>>>();
}
}
