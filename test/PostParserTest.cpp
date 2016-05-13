#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <folly/String.h>
#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPCommonHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>

#include "src/HTTPRequest.h"
#include "src/PostParser.h"
#include "src/StringUtils.h"
#include "test/Common.h"

using namespace std;
using namespace folly;
using namespace proxygen;

namespace sakura {
namespace test {

TEST(PostParserTest, parse_request_returns_empty_headers_on_missing_header) {
  auto req = make_request("/", HTTPMethod::GET);
  auto postData = PostParser::parseRequest(req, req.getBodyAsBytes());

  ASSERT_EQ(0, postData.size());
}

TEST(PostParserTest, parses_url_encoded_when_urlencoded_header_is_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  auto postData = PostParser::parseRequest(req, req.getBodyAsBytes());

  ASSERT_EQ(2, postData.size());
  ASSERT_EQ("value1", to_string(postData["key1"][0]));
  ASSERT_EQ("value2", to_string(postData["key2"][0]));
}

TEST(DISABLED_PostParserTest, parses_form_data_when_form_data_header_is_set) {
  auto boundary = "abcde";
  auto bodyString = sformat(
      "--{0}\n"
      "Content-Disposition: form-data; name=\"key1\"\n"
      "\n"
      "value1\n"
      "--{0}\n"
      "Content-Disposition: form-data; name=\"key2\"\n"
      "\n"
      "value2\n");
  auto body = IOBuf::copyBuffer(bodyString);
  auto req = make_request(
      "/", HTTPMethod::GET, body->clone(),
      {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, "multipart/form-data"}});
  auto postData = PostParser::parseRequest(req, req.getBodyAsBytes());

  ASSERT_EQ(2, postData.size());
  ASSERT_EQ("value1", to_string(postData["key1"][0]));
  ASSERT_EQ("value2", to_string(postData["key2"][0]));
}

TEST(PostParserTest, parse_request_returns_empty_if_content_type_is_not_valid) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2");
  auto req = make_request(
      "/", HTTPMethod::GET, body->clone(),
      {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, "application/json"}});
  auto postData = PostParser::parseRequest(req, req.getBodyAsBytes());

  ASSERT_EQ(0, postData.size());
}

TEST(PostParserTest, getString_returns_empty_if_not_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_FALSE((bool)post.getString("key3"));
}

TEST(PostParserTest, getString_returns_first_of_multiple_values_if_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_EQ("value1", *post.getString("key1"));
  ASSERT_EQ("value2", *post.getString("key2"));
}

TEST(PostParserTest, getBinary_returns_empty_if_not_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_FALSE((bool)post.getBinary("key3"));
}

TEST(PostParserTest, getBinary_returns_first_of_multiple_values_if_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_EQ("value1", to_string(*(post.getBinary("key1"))));
  ASSERT_EQ("value2", to_string(*(post.getBinary("key2"))));
}

TEST(PostParserTest, getStringList_returns_empty_if_not_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_FALSE((bool)post.getStringList("key3"));
}

TEST(PostParserTest, getStringList_returns_correctly_ordered_list_if_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  auto values1 = post.getStringList("key1");
  auto values2 = post.getStringList("key2");

  ASSERT_EQ(2, (*values1).size());
  ASSERT_EQ(1, (*values2).size());
  ASSERT_EQ("value1", (*values1)[0]);
  ASSERT_EQ("value3", (*values1)[1]);
  ASSERT_EQ("value2", (*values2)[0]);
}

TEST(PostParserTest, getBinaryList_returns_empty_if_not_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  ASSERT_FALSE((bool)post.getBinaryList("key3"));
}

TEST(PostParserTest, getBinaryList_returns_correctly_ordered_list_if_set) {
  auto body = IOBuf::copyBuffer("key1=value1&key2=value2&key1=value3");
  auto req = make_request("/", HTTPMethod::GET, body->clone(),
                          {{HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
                            "application/x-www-form-urlencoded"}});
  PostParser post(req);

  auto values1 = post.getBinaryList("key1");
  auto values2 = post.getBinaryList("key2");

  ASSERT_EQ(2, (*values1).size());
  ASSERT_EQ(1, (*values2).size());
  ASSERT_EQ("value1", to_string((*values1)[0]));
  ASSERT_EQ("value3", to_string((*values1)[1]));
  ASSERT_EQ("value2", to_string((*values2)[0]));
}

TEST(PostParserTest, parseUrlEncoded_sets_empty_value_for_missing_equal_sign) {
  string postString = "&&key+1=testing+string&key2=&&";
  auto body = IOBuf::copyBuffer(postString);
  auto post = PostParser::parseUrlEncoded(std::move(body));

  ASSERT_NE(post.end(), post.find("key 1"));
  ASSERT_NE(post.end(), post.find("key2"));
  ASSERT_EQ("testing string", to_string(post["key 1"][0]));
  ASSERT_EQ("", to_string(post["key2"][0]));
}

TEST(PostParserTest, parseUrlEncoded_removes_empty_keys) {
  string postString = "&&key+1=testing+string&&";
  auto body = IOBuf::copyBuffer(postString);
  auto post = PostParser::parseUrlEncoded(std::move(body));

  ASSERT_NE(post.end(), post.find("key 1"));
  ASSERT_EQ("testing string", to_string(post["key 1"][0]));
}

TEST(PostParserTest,
     parseUrlEncoded_does_not_url_decode_on_invalid_key_or_value) {
  string postString1 = "key+1=%GFtesting+string";
  string postString2 = "key%GF2+1=testing+string";
  auto body1 = IOBuf::copyBuffer(postString1);
  auto body2 = IOBuf::copyBuffer(postString2);
  auto post1 = PostParser::parseUrlEncoded(std::move(body1));
  auto post2 = PostParser::parseUrlEncoded(std::move(body2));

  ASSERT_NE(post1.end(), post1.find("key 1"));
  ASSERT_NE(post2.end(), post2.find("key%GF2+1"));
  ASSERT_EQ("%GFtesting+string", to_string(post1["key 1"][0]));
  ASSERT_EQ("testing string", to_string(post2["key%GF2+1"][0]));
}

TEST(
    PostParserTest,
    parseUrlEncoded_properly_decodes_and_sets_multiple_keys_with_one_or_more_values) {
  string postString = "key1=value1+value2%20value3&key%202=value2&key+3=&";
  string postString2 = "key1=second+value1&&";
  auto body = IOBuf::copyBuffer(postString);
  body->prependChain(IOBuf::copyBuffer(postString2));

  auto post = PostParser::parseUrlEncoded(std::move(body));

  ASSERT_NE(post.end(), post.find("key1"));
  ASSERT_NE(post.end(), post.find("key 2"));
  ASSERT_NE(post.end(), post.find("key 3"));
  ASSERT_EQ(post.end(), post.find(""));

  auto& v1 = post["key1"];
  auto& v2 = post["key 2"];
  auto& v3 = post["key 3"];

  ASSERT_EQ(2, v1.size());
  ASSERT_EQ(1, v2.size());
  ASSERT_EQ(1, v3.size());

  ASSERT_EQ("value1 value2 value3", to_string(v1[0]));
  ASSERT_EQ("second value1", to_string(v1[1]));
  ASSERT_EQ("value2", to_string(v2[0]));
  ASSERT_EQ("", to_string(v3[0]));
}
}
}
