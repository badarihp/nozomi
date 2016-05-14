#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <folly/SocketAddress.h>

#include "src/Config.h"
#include "test/Common.h"

using namespace std;

namespace nozomi {
namespace test {

TEST(ConfigTest, empty_address_list_throws) {
  ASSERT_THROW_MSG(
      { Config c(vector<tuple<string, uint16_t, Config::Protocol>>(), 1); },
      std::invalid_argument, "At least one address must be provided");
  ASSERT_THROW_MSG({ Config c(vector<proxygen::HTTPServer::IPConfig>(), 1); },
                   std::invalid_argument,
                   "At least one address must be provided");
}
TEST(ConfigTest, invalid_ipv4_address_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("255.255.255.256", 1234, Config::Protocol::HTTP)},
                 1);
      },
      std::invalid_argument,
      "Failed to resolve address for \"255.255.255.256\"");
}

TEST(ConfigTest, invalid_ipv6_address_throws) {
  ASSERT_THROW_MSG(
      { Config c({make_tuple(":::1", 1234, Config::Protocol::HTTP)}, 1); },
      std::invalid_argument, "Failed to resolve address for \":::1\"");
}

TEST(ConfigTest, zero_threads_throws) {
  ASSERT_THROW_MSG(
      { Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 0); },
      std::invalid_argument, "Number of threads (0) must be greater than zero");
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

  ASSERT_EQ("127.0.0.1", c.getHTTPAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c.getHTTPAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c.getHTTPAddresses()[0].protocol);
  ASSERT_EQ("::1", c.getHTTPAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c.getHTTPAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c.getHTTPAddresses()[1].protocol);
  ASSERT_EQ(1, c.getWorkerThreads());

  ASSERT_EQ("127.0.0.1", c2.getHTTPAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c2.getHTTPAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c2.getHTTPAddresses()[0].protocol);
  ASSERT_EQ("::1", c2.getHTTPAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c2.getHTTPAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c2.getHTTPAddresses()[1].protocol);
  ASSERT_EQ(1, c2.getWorkerThreads());
}
}
}
