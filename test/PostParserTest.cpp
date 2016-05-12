#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include <folly/io/IOBuf.h>

#include "src/PostParser.h"
#include "src/StringUtils.h"

using namespace std;
using namespace folly;

namespace sakura {
namespace test {

TEST(PostParserTest, parse_request_returns_empty_headers_on_missing_header) {

}

TEST(PostParserTest, parses_url_encoded_when_urlencoded_header_is_set) {

}

TEST(PostParserTest, parses_form_data_when_form_data_header_is_set) {

}

TEST(PostParserTest, parse_request_returns_empty_if_content_type_is_not_valid) {

}

TEST(PostParserTest, getString_returns_empty_if_not_set) {

}

TEST(PostParserTest, getString_returns_first_of_multiple_values_if_set) {

}

TEST(PostParserTest, getBinary_returns_empty_if_not_set) {

}

TEST(PostParserTest, getBinary_returns_first_of_multiple_values_if_set) {

}

TEST(PostParserTest, getStringList_returns_empty_if_not_set) {

}

TEST(PostParserTest, getStringList_returns_correctly_ordered_list_if_set) {

}
TEST(PostParserTest, getBinaryList_returns_empty_if_not_set) {

}

TEST(PostParserTest, getBinaryList_returns_correctly_ordered_list_if_set) {

}

TEST(PostParserTest, parseUrlEncoded_sets_empty_value_for_missing_equal_sign) {
  string postString = "&&key+1=testing+string&key2=&&";
  auto body = IOBuf::copyBuffer(postString);
  auto post = PostParser::parseUrlEncoded(std::move(body));
  
  ASSERT_NE(post.find("key 1"), post.end());
  ASSERT_NE(post.find("key2"), post.end());
  ASSERT_EQ("testing string",to_string(post["key 1"][0]));
  ASSERT_EQ("",to_string(post["key2"][0]));
}

TEST(PostParserTest, parseUrlEncoded_removes_empty_keys) {
  string postString = "&&key+1=testing+string&&";
  auto body = IOBuf::copyBuffer(postString);
  auto post = PostParser::parseUrlEncoded(std::move(body));
  
  ASSERT_NE(post.find("key 1"), post.end());
  ASSERT_EQ("testing string",to_string(post["key 1"][0]));
}

TEST(PostParserTest, parseUrlEncoded_does_not_url_decode_on_invalid_key_or_value) {
  string postString1 = "key+1=%GFtesting+string";
  string postString2 = "key%GF2+1=testing+string";
  auto body1 = IOBuf::copyBuffer(postString1);
  auto body2 = IOBuf::copyBuffer(postString2);
  auto post1 = PostParser::parseUrlEncoded(std::move(body1));
  auto post2 = PostParser::parseUrlEncoded(std::move(body2));
  
  ASSERT_NE(post1.find("key 1"), post1.end());
  ASSERT_NE(post2.find("key%GF2+1"), post2.end());
  ASSERT_EQ("%GFtesting+string",to_string(post1["key 1"][0]));
  ASSERT_EQ("testing string",to_string(post2["key%GF2+1"][0]));
} 

TEST(PostParserTest, parseUrlEncoded_properly_decodes_and_sets_multiple_keys_with_one_or_more_values) {
  string postString = "key1=value1+value2%20value3&key%202=value2&key+3=&";
  string postString2 = "key1=second+value1&&";
  auto body = IOBuf::copyBuffer(postString);
  body->prependChain(IOBuf::copyBuffer(postString2));

  auto post = PostParser::parseUrlEncoded(std::move(body));

  ASSERT_NE(post.find("key1"), post.end());
  ASSERT_NE(post.find("key 2"), post.end());
  ASSERT_NE(post.find("key 3"), post.end());
  ASSERT_EQ(post.find(""), post.end());

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
