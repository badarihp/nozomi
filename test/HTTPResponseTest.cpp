#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <folly/io/IOBuf.h>
#include <folly/json.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMessage.h>

#include "src/HTTPResponse.h"

using namespace std;
using namespace folly;
using namespace proxygen;

namespace sakura {
namespace test {

TEST(HTTPResponseTest, constructors_and_getters_work) {
  vector<HTTPResponse> responses;
  responses.push_back(HTTPResponse(201, parseJson(R"({"test":"value"})"),
                                   {{"Location", "http://google.com"}}));
  responses.push_back(HTTPResponse(201, string(R"({"test":"value"})"),
                                   {{"Location", "http://google.com"}}));
  responses.push_back(HTTPResponse(201,
                                   IOBuf::copyBuffer(R"({"test":"value"})"),
                                   {{"Location", "http://google.com"}}));
  responses.push_back(HTTPResponse(
      201, parseJson(R"({"test":"value"})"),
      {{HTTPHeaderCode::HTTP_HEADER_LOCATION, "http://google.com"}}));
  responses.push_back(HTTPResponse(
      201, string(R"({"test":"value"})"),
      {{HTTPHeaderCode::HTTP_HEADER_LOCATION, "http://google.com"}}));
  responses.push_back(HTTPResponse(
      201, IOBuf::copyBuffer(R"({"test":"value"})"),
      {{HTTPHeaderCode::HTTP_HEADER_LOCATION, "http://google.com"}}));

  for (const auto& response : responses) {
    ASSERT_EQ(201, response.getStatusCode());
    ASSERT_EQ(R"({"test":"value"})", response.getBodyString());
    ASSERT_EQ("http://google.com",
              response.getHeaders().getHeaders().getSingleOrEmpty(
                  HTTPHeaderCode::HTTP_HEADER_LOCATION));
  }
}
}
}
