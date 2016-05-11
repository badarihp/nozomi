#include "src/PostParser.h"

#include "src/StringUtils.h"

#include <folly/gen/Base.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>

using folly::IOBuf;
using folly::Optional;
using std::unordered_map;
using std::string;
using std::unique_ptr;
using std::vector;
using proxygen::HTTPHeaderCode;
using namespace folly::gen;

namespace sakura {
PostParser::PostParser(const HTTPRequest& request)
    : originalBody_(request.getBodyAsBytes()), parsedData_(parseRequest(request, originalBody_)) {}

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
    return Optional<vector<string>>();
//        from(it->second) | map([](const auto& buf) { return to_string(buf); }) |
//        as < vector<string>>());
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
    return Optional<vector<unique_ptr<IOBuf>>>();
//        from(it->second) | map([](const auto& buf) { return buf->clone(); }) |
//        as < vector<string>>());
  };
}

unordered_map<string, vector<unique_ptr<IOBuf>>> PostParser::parseUrlEncoded(
    const unique_ptr<IOBuf>& body) {
  return unordered_map<string, vector<unique_ptr<IOBuf>>>();
}

unordered_map<string, vector<unique_ptr<IOBuf>>> PostParser::parseFormData(
    const unique_ptr<IOBuf>& body) {
  return unordered_map<string, vector<unique_ptr<IOBuf>>>();
}
}
