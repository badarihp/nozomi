#pragma once

#include <memory>
#include <string>

#include <folly/String.h>
#include <folly/io/IOBuf.h>
#include <folly/json.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/StringUtils.h"

namespace nozomi {

class HTTPRequest {
 public:
  struct QueryParams {
    QueryParams(const proxygen::HTTPMessage* request) : request_(request) {
      DCHECK(request != nullptr);
    }
    inline folly::Optional<std::string> getQueryParam(
        const std::string& param) const {
      if (request_->hasQueryParam(param)) {
        return request_->getDecodedQueryParam(param);
      }
      for (const auto& kvp : request_->getQueryParams()) {
        try {
          auto decodedParam = folly::uriUnescape<std::string>(
              kvp.first, folly::UriEscapeMode::QUERY);
          if (decodedParam == param) {
            return folly::uriUnescape<std::string>(kvp.second,
                                                   folly::UriEscapeMode::QUERY);
          }
        } catch (const std::exception& e) {
          // TODO: Log
          continue;
        }
      }

      return folly::Optional<std::string>();
    }
    inline folly::Optional<std::string> operator[](
        const std::string& param) const {
      return getQueryParam(param);
    }

   private:
    const proxygen::HTTPMessage* request_;
  };

  struct Headers {
    Headers(const proxygen::HTTPMessage* request) : request_(request) {
      DCHECK(request != nullptr);
    }
    template <typename HeaderKeyType>
    inline folly::Optional<std::string> getHeader(
        const HeaderKeyType& param) const {
      const auto& value = request_->getHeaders().getSingleOrEmpty(param);
      if (value.empty()) {
        return folly::Optional<std::string>();
      } else {
        return folly::Optional<std::string>(value);
      }
    }
    inline folly::Optional<std::string> getHeader(const char* param) const {
      return getHeader(folly::StringPiece(param));
    }

    template <typename HeaderKeyType>
    inline folly::Optional<std::string> operator[](
        const HeaderKeyType& param) const {
      return getHeader(param);
    }
    inline folly::Optional<std::string> operator[](const char* param) const {
      return getHeader(folly::StringPiece(param));
    }

   private:
    const proxygen::HTTPMessage* request_;
  };

  struct Cookies {
    Cookies(const proxygen::HTTPMessage* request) : request_(request) {
      DCHECK(request != nullptr);
    }
    inline folly::Optional<std::string> getCookie(
        const std::string& param) const {
      auto value = request_->getCookie(param);
      if (value.empty()) {
        return folly::Optional<std::string>();
      } else {
        return folly::Optional<std::string>(value.str());
      }
    }
    inline folly::Optional<std::string> operator[](
        const std::string& param) const {
      return getCookie(param);
    }

   private:
    const proxygen::HTTPMessage* request_;
  };

  HTTPRequest(std::unique_ptr<proxygen::HTTPMessage> request,
              std::unique_ptr<folly::IOBuf> body);
  inline const std::string& getPath() const { return path_; }
  inline proxygen::HTTPMethod getMethod() const { return method_; }

  inline const proxygen::HTTPHeaders& getRawHeaders() const {
    return request_->getHeaders();
  }
  inline const proxygen::HTTPMessage& getRawRequest() const {
    return *request_;
  }
  inline std::string getBodyAsString() const { return to_string(body_); }
  folly::dynamic getBodyAsJson() const {
    return folly::parseJson(getBodyAsString());
  }
  inline std::unique_ptr<folly::IOBuf> getBodyAsBytes() const {
    return body_->clone();
  }
  // TODO: Proxygen all the things

  inline const QueryParams& getQueryParams() const { return queryParams_; }
  inline const Headers& getHeaders() const { return headers_; }
  inline const Cookies& getCookies() const { return cookies_; }

  static std::tuple<proxygen::HTTPMethod, std::string> getMethodAndPath(
      const proxygen::HTTPMessage* message);

 private:
  std::unique_ptr<proxygen::HTTPMessage> request_;
  std::unique_ptr<folly::IOBuf> body_;
  std::string path_;
  QueryParams queryParams_;
  Headers headers_;
  Cookies cookies_;
  proxygen::HTTPMethod method_;

  static inline std::string getUnescapedPath(const std::string& originalPath) {
    try {
      return folly::uriUnescape<std::string>(originalPath,
                                             folly::UriEscapeMode::PATH);
    } catch (const std::exception& e) {
      return originalPath;
    }
  }
};
}
