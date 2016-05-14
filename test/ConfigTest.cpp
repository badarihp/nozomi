#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <folly/SocketAddress.h>

#include "src/Config.h"

using namespace std;

namespace nozomi {
namespace test {

TEST(ConfigTest, empty_address_list_throws) {
  EXPECT_THROW(
      { Config c(vector<tuple<string, uint16_t, Config::Protocol>>(), 1); },
      std::invalid_argument);
  EXPECT_THROW({ Config c(vector<proxygen::HTTPServer::IPConfig>(), 1); },
               std::invalid_argument);
}

TEST(ConfigTest, invalid_ipv4_address_throws) {
  EXPECT_THROW(
      {
        Config c({make_tuple("255.255.255.256", 1234, Config::Protocol::HTTP)},
                 1);
      },
      std::invalid_argument);
}

TEST(ConfigTest, invalid_ipv6_address_throws) {
  EXPECT_THROW(
      { Config c({make_tuple(":::1", 1234, Config::Protocol::HTTP)}, 1); },
      std::invalid_argument);
}

TEST(ConfigTest, zero_threads_throws) {
  EXPECT_THROW(
      { Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 0); },
      std::invalid_argument);
}

TEST(ConfigTest, constructors_work) {
  vector<string> addresses{
      "127.0.0.1", "::1",
  };
  Config c(
      {
          make_tuple("127.0.0.1", 8080, Config::Protocol::HTTP),
          make_tuple("::1", 8081, Config::Protocol::HTTP2),
      },
      1);
  Config c2(
      {
          proxygen::HTTPServer::IPConfig(
              folly::SocketAddress("127.0.0.1", 8080), Config::Protocol::HTTP),
          proxygen::HTTPServer::IPConfig(folly::SocketAddress("::1", 8081),
                                         Config::Protocol::HTTP2),
      },
      1);

  ASSERT_EQ("127.0.0.1", c.getHttpAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c.getHttpAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c.getHttpAddresses()[0].protocol);
  ASSERT_EQ("::1", c.getHttpAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c.getHttpAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c.getHttpAddresses()[1].protocol);
  ASSERT_EQ(1, c.getWorkerThreads());

  ASSERT_EQ("127.0.0.1", c2.getHttpAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c2.getHttpAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c2.getHttpAddresses()[0].protocol);
  ASSERT_EQ("::1", c2.getHttpAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c2.getHttpAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c2.getHttpAddresses()[1].protocol);
  ASSERT_EQ(1, c2.getWorkerThreads());
}
}
}
