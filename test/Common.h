#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/regex.hpp>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/EnumHash.h"
#include "src/HTTPRequest.h"

namespace nozomi {
namespace test {

inline ::testing::AssertionResult assertThrowMsg(
    const char* expr, std::function<::testing::AssertionResult()> f) {
  return f();
}

#define ASSERT_THROW_MSG(BLOCK, EXCEPTION, MESSAGE)                            \
                                                                               \
  ASSERT_PRED_FORMAT1(                                                         \
      nozomi::test::assertThrowMsg, [&]() -> ::testing::AssertionResult {      \
        try {                                                                  \
          BLOCK;                                                               \
          return ::testing::AssertionFailure()                                 \
                 << "did not get an exception of type " << #EXCEPTION;         \
        } catch (const EXCEPTION& e) {                                         \
          std::string msg(e.what());                                           \
          if (msg.find(MESSAGE) == std::string::npos &&                        \
              !boost::regex_match(                                             \
                  e.what(), boost::basic_regex<char>("(\\s?)" MESSAGE))) {     \
            return ::testing::AssertionFailure()                               \
                   << "Message \"" << e.what() << "\""                         \
                   << " did not match expected pattern \"" << MESSAGE << "\""; \
          } else {                                                             \
            return ::testing::AssertionSuccess();                              \
          }                                                                    \
        } catch (const std::exception& e) {                                    \
          return ::testing::AssertionFailure()                                 \
                 << "did not get an exception of type " << #EXCEPTION;         \
        }                                                                      \
      });

template <typename HeaderType = proxygen::HTTPHeaderCode>
HTTPRequest make_request(
    std::string path,
    proxygen::HTTPMethod method = proxygen::HTTPMethod::GET,
    std::unique_ptr<folly::IOBuf> body = folly::IOBuf::create(0),
    std::unordered_map<HeaderType, std::string> headers =
        std::unordered_map<HeaderType, std::string>()) {
  auto message = std::make_unique<proxygen::HTTPMessage>();
  message->setURL(path);
  message->setMethod(method);
  for (const auto& kvp : headers) {
    message->getHeaders().set(kvp.first, kvp.second);
  }
  return HTTPRequest(std::move(message), std::move(body));
}
}
}
