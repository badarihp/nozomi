#include <gtest/gtest.h>

#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <folly/io/IOBuf.h>

#include "src/HTTPRequest.h"

using proxygen::HTTPMessage;
using proxygen::HTTPHeaders;
using proxygen::HTTPMethod;
using proxygen::HTTPHeaderCode;
using folly::IOBuf;

namespace sakura {
namespace test {

TEST(HTTPRequestTest, status_code_is_correct) {
  HTTPMessage message;
  message.setStatusCode(404);
  HTTPRequest request(std::move(message), IOBuf::create(0));

  ASSERT_EQ(404, request.getStatusCode());
}

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
  message.setURL("/testing%%20path+here");
  HTTPRequest request(std::move(message), IOBuf::create(0));
  
  ASSERT_EQ("/testing path+here", request.getPath());
}

TEST(HTTPRequestTest, get_path_doesnt_decode_broken_uri_encoded_paths) {
  HTTPMessage message;
  message.setURL("/testing%%ffpath%20");
  HTTPRequest request(std::move(message), IOBuf::create(0));

  ASSERT_EQ("/testing%%ffpath%20", request.getPath());
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

}

TEST(HTTPRequestTest, json_decoding_throws_on_failure) {

}

TEST(HTTPRequestTest, json_decoding_works) {

}

TEST(HTTPRequestTest, returns_empty_query_param_when_not_set) {

}

TEST(HTTPRequestTest, returns_decoded_query_param_when_set) {

}

TEST(HTTPRequestTest, returns_empty_value_when_header_not_set) {

}

TEST(HTTPRequestTest, returns_header_value_when_set) {

}

TEST(HTTPRequestTest, header_code_works_for_getters) {

}

TEST(HTTPRequestTest, returns_empty_cookie_when_not_set) {

}

TEST(HTTPRequestTest, returns_cookie_when_set) {

}

}
}
