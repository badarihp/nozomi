#pragma once

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/HTTPRequest.h"

namespace sakura {

class PostParser {
 private:
  std::unique_ptr<folly::IOBuf> originalBody_;
  std::unordered_map<std::string, std::vector<std::unique_ptr<folly::IOBuf>>>
      parsedData_;

 public:
  PostParser(const HTTPRequest& request);
  folly::Optional<std::string> getString(const std::string& key);
  folly::Optional<std::vector<std::string>> getStringList(
      const std::string& key);
  folly::Optional<std::unique_ptr<folly::IOBuf>> getBinary(
      const std::string& key);
  folly::Optional<std::vector<std::unique_ptr<folly::IOBuf>>> getBinaryList(
      const std::string& key);

  static std::unordered_map<std::string,
                            std::vector<std::unique_ptr<folly::IOBuf>>>
  parseUrlEncoded(const std::unique_ptr<folly::IOBuf>& body);
  static std::unordered_map<std::string,
                            std::vector<std::unique_ptr<folly::IOBuf>>>
  parseFormData(const std::unique_ptr<folly::IOBuf>& body);
  static std::unordered_map<std::string,
                            std::vector<std::unique_ptr<folly::IOBuf>>>
  parseRequest(const HTTPRequest& request,
               const std::unique_ptr<folly::IOBuf>& body);
};
}
