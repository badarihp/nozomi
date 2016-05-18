#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>

#include "src/Config.h"
#include "test/Common.h"

using boost::filesystem::path;
using folly::Optional;
using namespace std;
namespace fs = boost::filesystem;

namespace nozomi {
namespace test {

TEST(ConfigTest, empty_address_list_throws) {
  ASSERT_THROW_MSG(
      {
        Config c(vector<tuple<string, uint16_t, Config::Protocol>>(), 1,
                 Optional<string>());
      },
      std::invalid_argument, "At least one address must be provided");
  ASSERT_THROW_MSG({ Config c(vector<proxygen::HTTPServer::IPConfig>(), 1); },
                   std::invalid_argument,
                   "At least one address must be provided");
}
TEST(ConfigTest, invalid_ipv4_address_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("255.255.255.256", 1234, Config::Protocol::HTTP)},
                 1, Optional<string>());
      },
      std::invalid_argument,
      "Failed to resolve address for \"255.255.255.256\"");
}

TEST(ConfigTest, invalid_ipv6_address_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple(":::1", 1234, Config::Protocol::HTTP)}, 1,
                 Optional<string>());
      },
      std::invalid_argument, "Failed to resolve address for \":::1\"");
}

TEST(ConfigTest, zero_threads_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 0,
                 Optional<string>());
      },
      std::invalid_argument, "Number of threads (0) must be greater than zero");
}

TEST(ConfigTest, zero_buffer_size_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 Optional<string>(), std::chrono::milliseconds(45), 0);
      },
      std::invalid_argument,
      "The file reader buffer size (0) must be > 0 and less than 1GB");

  ASSERT_THROW_MSG(
      {
        Config c({proxygen::HTTPServer::IPConfig(
                     folly::SocketAddress("127.0.0.1", 8080),
                     Config::Protocol::HTTP)},
                 1, Optional<string>(), std::chrono::milliseconds(45), 0);
      },
      std::invalid_argument,
      "The file reader buffer size (0) must be > 0 and less than 1GB");
}

TEST(ConfigTest, four_GB_buffer_size_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 Optional<string>(), std::chrono::milliseconds(45), 4294967295);
      },
      std::invalid_argument,
      "The file reader buffer size (4294967295) must be > 0 and less than 1GB");

  ASSERT_THROW_MSG(
      {
        Config c({proxygen::HTTPServer::IPConfig(
                     folly::SocketAddress("127.0.0.1", 8080),
                     Config::Protocol::HTTP)},
                 1, Optional<string>(), std::chrono::milliseconds(45),
                 4294967295);
      },
      std::invalid_argument,
      "The file reader buffer size (4294967295) must be > 0 and less than 1GB");
}

TEST(ConfigTest, zero_timeout_throws) {
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 Optional<string>(), std::chrono::milliseconds(-1));
      },
      std::invalid_argument,
      "Timeout (-1) must be greater than zero milliseconds");
  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 Optional<string>(), std::chrono::milliseconds(0));
      },
      std::invalid_argument,
      "Timeout (0) must be greater than zero milliseconds");
}

TEST(ConfigTest, public_dir_doesnt_exist_throws_error) {
  TempDir tempDir;
  auto publicDir = (tempDir.tempDir / "invalid").string();

  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 publicDir);
      },
      std::invalid_argument, "Could not use public directory");

  ASSERT_THROW_MSG(
      {
        Config c({proxygen::HTTPServer::IPConfig(
                     folly::SocketAddress("127.0.0.1", 8080),
                     Config::Protocol::HTTP)},
                 1, publicDir);
      },
      std::invalid_argument, "Could not use public directory");
}

TEST(ConfigTest, public_dir_as_symlink_to_non_directory_throws) {
  TempDir tempDir;
  auto file = tempDir.tempDir / "tempFile";
  auto link = tempDir.tempDir / "link";
  auto publicDir = link.string();

  ofstream fout(file.string());
  fout << "Data";
  fout.close();
  fs::create_symlink(file, link);

  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 publicDir);
      },
      std::invalid_argument, "Could not use public directory");

  ASSERT_THROW_MSG(
      {
        Config c({proxygen::HTTPServer::IPConfig(
                     folly::SocketAddress("127.0.0.1", 8080),
                     Config::Protocol::HTTP)},
                 1, publicDir);
      },
      std::invalid_argument, "Could not use public directory");
}

TEST(ConfigTest, public_dir_as_file_throws_error) {
  TempDir tempDir;
  auto file = tempDir.tempDir / "tempFile";
  auto publicDir = file.string();

  ofstream fout(file.string());
  fout << "Data";
  fout.close();

  ASSERT_THROW_MSG(
      {
        Config c({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
                 publicDir);
      },
      std::invalid_argument, "Could not use public directory");

  ASSERT_THROW_MSG(
      {
        Config c({proxygen::HTTPServer::IPConfig(
                     folly::SocketAddress("127.0.0.1", 8080),
                     Config::Protocol::HTTP)},
                 1, publicDir);
      },
      std::invalid_argument, "Could not use public directory");
}

TEST(ConfigTest, public_dir_as_symlink_to_directory_works) {
  TempDir tempDir;
  tempDir.deleteFiles = false;
  auto dir = tempDir.tempDir / "tempDir2";
  auto link = tempDir.tempDir / "link";
  auto publicDir = link.string();

  fs::create_directory(dir);
  fs::create_symlink(dir, link);

  Config c1({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1, publicDir);

  Config c2(
      {proxygen::HTTPServer::IPConfig(folly::SocketAddress("127.0.0.1", 8080),
                                      Config::Protocol::HTTP)},
      1, publicDir);

  ASSERT_EQ(dir, c1.getPublicDirectory().value());
  ASSERT_EQ(dir, c2.getPublicDirectory().value());
}

TEST(ConfigTest, empty_path_returns_empty_public_dir) {
  Config c1({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1,
            Optional<string>());
  Config c2(
      {proxygen::HTTPServer::IPConfig(folly::SocketAddress("127.0.0.1", 8080),
                                      Config::Protocol::HTTP)},
      1, Optional<string>());
  ASSERT_FALSE(c1.getPublicDirectory());
  ASSERT_FALSE(c2.getPublicDirectory());
}

TEST(ConfigTest, existing_public_dir_returns_public_dir) {
  TempDir tempDir;
  auto dir = tempDir.tempDir / "tempDir2";
  fs::create_directory(dir);

  Config c1({make_tuple("::1", 1234, Config::Protocol::HTTP)}, 1, dir.string());

  Config c2(
      {proxygen::HTTPServer::IPConfig(folly::SocketAddress("127.0.0.1", 8080),
                                      Config::Protocol::HTTP)},
      1, dir.string());
  ASSERT_EQ(dir, c1.getPublicDirectory().value());
  ASSERT_EQ(dir, c2.getPublicDirectory().value());
}

TEST(ConfigTest, constructors_work) {
  TempDir tempDir;

  vector<string> addresses{
      "127.0.0.1", "::1",
  };
  Config c(
      {
          make_tuple("127.0.0.1", 8080, Config::Protocol::HTTP),
          make_tuple("::1", 8081, Config::Protocol::HTTP2),
      },
      1, tempDir.tempDir.string(), std::chrono::milliseconds(45), 100);
  Config c2(
      {
          proxygen::HTTPServer::IPConfig(
              folly::SocketAddress("127.0.0.1", 8080), Config::Protocol::HTTP),
          proxygen::HTTPServer::IPConfig(folly::SocketAddress("::1", 8081),
                                         Config::Protocol::HTTP2),
      },
      1, tempDir.tempDir.string(), std::chrono::milliseconds(45), 100);

  ASSERT_EQ("127.0.0.1", c.getHTTPAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c.getHTTPAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c.getHTTPAddresses()[0].protocol);
  ASSERT_EQ("::1", c.getHTTPAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c.getHTTPAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c.getHTTPAddresses()[1].protocol);
  ASSERT_EQ(1, c.getWorkerThreads());
  ASSERT_EQ(tempDir.tempDir, *(c.getPublicDirectory()));
  ASSERT_EQ(std::chrono::milliseconds(45), c.getRequestTimeout());
  ASSERT_EQ(100, c.getFileReaderBufferSize());

  ASSERT_EQ("127.0.0.1", c2.getHTTPAddresses()[0].address.getAddressStr());
  ASSERT_EQ(8080, c2.getHTTPAddresses()[0].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP, c2.getHTTPAddresses()[0].protocol);
  ASSERT_EQ("::1", c2.getHTTPAddresses()[1].address.getAddressStr());
  ASSERT_EQ(8081, c2.getHTTPAddresses()[1].address.getPort());
  ASSERT_EQ(Config::Protocol::HTTP2, c2.getHTTPAddresses()[1].protocol);
  ASSERT_EQ(1, c2.getWorkerThreads());
  ASSERT_EQ(tempDir.tempDir, *(c2.getPublicDirectory()));
  ASSERT_EQ(std::chrono::milliseconds(45), c2.getRequestTimeout());
  ASSERT_EQ(100, c2.getFileReaderBufferSize());
}
}
}
