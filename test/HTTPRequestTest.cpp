#include <gtest/gtest.h>

#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <folly/io/IOBuf.h>
#include <folly/dynamic.h>

#include "src/HTTPRequest.h"

using proxygen::HTTPMessage;
using proxygen::HTTPHeaders;
using proxygen::HTTPMethod;
using proxygen::HTTPHeaderCode;
using folly::IOBuf;

namespace sakura {
namespace test {

TEST(HTTPRequestTest, method_is_correct_or_defaults_to_get) {
  HTTPMessage message1;
  HTTPMessage message2;
  message1.setMethod(HTTPMethod::POST);
  message2.setMethod("INVALID_METHOD");
  HTTPRequest request1(std::move(message1), IOBuf::create(0));
  HTTPRequest request2(std::move(message2), IOBuf::create(0));

  ASSERT_EQ(HTTPMethod::POST, request1.getMethod());
  ASSERT_EQ(HTTPMethod::GET, request2.getMethod());
}

TEST(HTTPRequestTest, get_path_works_and_decodes_uri_escaping) {
  HTTPMessage message;
  message.setURL("/testing%20path+here");
  HTTPRequest request(std::move(message), IOBuf::create(0));
  
  ASSERT_EQ("/testing path+here", request.getPath());
}

TEST(HTTPRequestTest, get_path_doesnt_decode_broken_uri_encoded_paths) {
  HTTPMessage message;
  message.setURL("/testing%GGpath%20");
  HTTPRequest request(std::move(message), IOBuf::create(0));

  ASSERT_EQ("/testing%GGpath%20", request.getPath());
}

TEST(HTTPRequestTest, headers_are_returned) {
  HTTPMessage message;
  message.getHeaders().set("Location", "/index.php");
  HTTPRequest request(std::move(message), IOBuf::create(0));

  ASSERT_EQ("/index.php", request.getHeaders()["Location"].value());
  ASSERT_EQ("/index.php", request.getHeaders()[HTTPHeaderCode::HTTP_HEADER_LOCATION].value());
  ASSERT_EQ("/index.php", request.getHeaders().getHeader("Location").value());
  ASSERT_EQ("/index.php", request.getHeaders().getHeader(HTTPHeaderCode::HTTP_HEADER_LOCATION).value());
}

TEST(HTTPRequestTest, proxygen_http_message_is_returned) {
  HTTPMessage message;
  message.setURL(R"(/testing%20path+here)");
  HTTPRequest request(std::move(message), IOBuf::create(0));
 
  const auto& newMessage = request.getRawRequest();
 
  ASSERT_EQ(R"(/testing%20path+here)", newMessage.getURL());
}

TEST(HTTPRequestTest, proxygen_http_headers_are_returned) {
  HTTPMessage message;
  message.getHeaders().set("Location", "/index.php");
  HTTPRequest request(std::move(message), IOBuf::create(0));

  const auto& headers = request.getRawHeaders();

  ASSERT_EQ("/index.php", headers.getSingleOrEmpty("Location"));
  ASSERT_EQ("", headers.getSingleOrEmpty("Content-Type"));
}

TEST(HTTPRequestTest, body_as_string_traverses_chains) {
  const char* s1 = "The first string";
  const char* s2 = "The second string";
  auto buf1 = IOBuf::wrapBuffer(s1, strlen(s1));
  auto buf2 = IOBuf::wrapBuffer(s2, strlen(s2));
  buf1->prependChain(std::move(buf2));

  HTTPMessage message;
  HTTPRequest request(std::move(message), std::move(buf1));

  ASSERT_EQ("The first stringThe second string", request.getBodyAsString());
}

TEST(HTTPRequestTest, json_decoding_throws_on_failure) {
  const char* s1 = "The first string";
  const char* s2 = "The second string";
  auto buf1 = IOBuf::wrapBuffer(s1, strlen(s1));
  auto buf2 = IOBuf::wrapBuffer(s2, strlen(s2));
  buf1->prependChain(std::move(buf2));

  HTTPMessage message;
  HTTPRequest request(std::move(message), std::move(buf1));
  ASSERT_THROW({ request.getBodyAsJson(); }, std::runtime_error);
}

TEST(HTTPRequestTest, json_decoding_works) {
  const char* s1 = "{\"key\": ";
  const char* s2 = "\"value\"}";
  auto buf1 = IOBuf::wrapBuffer(s1, strlen(s1));
  auto buf2 = IOBuf::wrapBuffer(s2, strlen(s2));
  buf1->prependChain(std::move(buf2));

  HTTPMessage message;
  HTTPRequest request(std::move(message), std::move(buf1));

  auto json = request.getBodyAsJson();
  ASSERT_EQ("value", json["key"].asString());
}

TEST(HTTPRequestTest, returns_body_as_raw_iobuf) {
const char* s = "Hello, world";
  auto buf = IOBuf::wrapBuffer(s, strlen(s));

  HTTPMessage message;
  HTTPRequest request(std::move(message), std::move(buf));

  auto body = request.getBodyAsBytes();
  ASSERT_EQ(0, memcmp(s, body->data(), body->length()));
} 

TEST(HTTPRequestTest, returns_empty_query_param_when_not_set) {
  HTTPMessage message;
  HTTPRequest request(std::move(message), IOBuf::create(0));
  ASSERT_FALSE(request.getQueryParams()["arg1"].hasValue());
}

TEST(HTTPRequestTest, returns_decoded_query_param_when_set) {
  HTTPMessage message;
  message.setURL("/index.php?test%20variable=value%26&key=value");
  HTTPRequest request(std::move(message), IOBuf::create(0));
  ASSERT_EQ("value&", request.getQueryParams()["test variable"].value());
  ASSERT_EQ("value", request.getQueryParams()["key"].value());
}

TEST(HTTPRequestTest, skips_badly_encoded_params) {
  HTTPMessage message1;
  HTTPMessage message2;
  message1.setURL("/index.php?test%=value");
  message2.setURL("/index.php?test%20value=value%");

  HTTPRequest request1(std::move(message1), IOBuf::create(0));
  HTTPRequest request2(std::move(message2), IOBuf::create(0));

  ASSERT_FALSE(request1.getQueryParams()["test "].hasValue());
  ASSERT_FALSE(request2.getQueryParams()["test value"].hasValue());
}

TEST(HTTPRequestTest, returns_empty_value_when_header_not_set) {
  HTTPMessage message;
  HTTPRequest request(std::move(message), IOBuf::create(0));
  ASSERT_FALSE(request.getHeaders()["Location"].hasValue());
  ASSERT_FALSE(request.getHeaders()[HTTPHeaderCode::HTTP_HEADER_LOCATION].hasValue());
}

TEST(HTTPRequestTest, returns_header_value_when_set) {
  HTTPMessage message1;
  message1.getHeaders().set("Location", "val1");
  HTTPRequest request1(std::move(message1), IOBuf::create(0));

  HTTPMessage message2;
  message2.getHeaders().set(HTTPHeaderCode::HTTP_HEADER_LOCATION, "val2");
  HTTPRequest request2(std::move(message2), IOBuf::create(0));

  ASSERT_EQ("val1", request1.getHeaders()["Location"].value());
  ASSERT_EQ("val2", request2.getHeaders()[HTTPHeaderCode::HTTP_HEADER_LOCATION].value());
}

TEST(HTTPRequestTest, returns_empty_cookie_when_not_set) {
  HTTPMessage message;
  HTTPRequest request(std::move(message), IOBuf::create(0));
  ASSERT_FALSE(request.getCookies()["arg1"].hasValue());
}

TEST(HTTPRequestTest, returns_cookie_when_set) {
  HTTPMessage message;
  message.getHeaders().set("Cookie", "key1=value1;key2=value2;");
  HTTPRequest request(std::move(message), IOBuf::create(0));
  ASSERT_EQ("value1", request.getCookies()["key1"].value());
  ASSERT_EQ("value2", request.getCookies()["key2"].value());
}

}
}
