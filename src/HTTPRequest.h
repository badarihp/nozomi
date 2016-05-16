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

/**
 * Represents an HTTP requests's metadata and body
 */
class HTTPRequest {
 public:
  /**
   * Wraps up query parameters and does absence checking + url
   * decoding
   */
  struct QueryParams {
    QueryParams(const proxygen::HTTPMessage* request) : request_(request) {
      DCHECK(request != nullptr);
    }
    /**
     * Get a query parameter by name, or empty optional if the key
     * does not have an associated value
     */
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

  /**
   * Wraps up headers into a more easily queriable object that returns
   * Optionals, rather than empty strings
   */
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

  /**
   * Wraps up cookies into a more easily queriable object that returns
   * Optionals, rather than empty strings
   */
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

  /**
   * Returns the uri decoded path
   */
  inline const std::string& getPath() const { return path_; }

  /**
   * Returns the method that the request was made with, or GET
   * if the method was not able to be determined
   */
  inline proxygen::HTTPMethod getMethod() const { return method_; }

  /**
   * Gets the raw proxygen headers object
   */
  inline const proxygen::HTTPHeaders& getRawHeaders() const {
    return request_->getHeaders();
  }

  /**
   * Gets the raw proxygen::HTTPMessage object
   */
  inline const proxygen::HTTPMessage& getRawRequest() const {
    return *request_;
  }

  /**
   * Gets the body as an std::string
   */
  inline std::string getBodyAsString() const { return to_string(body_); }

  /**
   * Gets the body as a json object
   *
   * @throws runtime_error is thrown from folly::parseJson
   */
  folly::dynamic getBodyAsJson() const {
    return folly::parseJson(getBodyAsString());
  }

  /**
   * Gets the raw bytes from the body of the request. This
   * may be a chained IOBuf
   */
  inline std::unique_ptr<folly::IOBuf> getBodyAsBytes() const {
    return body_->clone();
  }

  inline const QueryParams& getQueryParams() const { return queryParams_; }
  inline const Headers& getHeaders() const { return headers_; }
  inline const Cookies& getCookies() const { return cookies_; }

  /**
   * Gets the method and path (unescaped) for a given HTTPMessage
   */
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
