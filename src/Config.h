#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <folly/Format.h>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>

namespace nozomi {

class Config {
 public:
  static constexpr size_t kDefaultFileReaderBufferSize = 4096;
  static constexpr int64_t kDefaultRequestTimeoutMs = 30000;
  using Protocol = proxygen::HTTPServer::Protocol;

 private:
  std::vector<proxygen::HTTPServer::IPConfig> httpAddresses_;
  size_t workerThreads_;
  boost::filesystem::path publicDir_;
  std::chrono::milliseconds requestTimeout_;
  size_t fileReaderBufferSize_;
  bool addPublicDirectoryHandler_ = false;

  void setHTTPAddresses(
      std::vector<proxygen::HTTPServer::IPConfig> httpAddresses);
  void setWorkerThreads(size_t workerThreads);
  void setRequestTimeout(std::chrono::milliseconds timeout);
  void setFileReaderBufferSize(size_t size);
  void setPublicDir(const folly::Optional<std::string>& path);

 public:
  /**
   * Creates a Config object for Servers
   * @param httpAddresses - A list of host / port / protocols to listen on
   * @param workerThreads - The number of threads to use for running event
   *                        handlers
   * @param publicDir - If provided, the directory to try to serve files
   *                    out of if all other routes are exhausted
   * @param requestTimeout - How long a single non-streaming request can
   *                         take to complete
   * @param fileReaderBufferSize - If static files are served, how large
   *                               the read buffer should be. Not usually
   *                               modified
   * @throws std::invalid_argument if any of the settings are not valid
   */
  Config(
      std::vector<std::tuple<std::string, uint16_t, Protocol>> httpAddresses,
      size_t workerThreads,
      folly::Optional<std::string> publicDir = folly::Optional<std::string>(),
      std::chrono::milliseconds requestTimeout =
          std::chrono::milliseconds(kDefaultRequestTimeoutMs),
      size_t fileReaderBufferSize = kDefaultFileReaderBufferSize);
  /**
   * Creates a Config object for Servers
   * @param httpAddresses - A list of host / port / protocols to listen on
   * @param workerThreads - The number of threads to use for running event
   *                        handlers
   * @param publicDir - If provided, the directory to try to serve files
   *                    out of if all other routes are exhausted
   * @param requestTimeout - How long a single non-streaming request can
   *                         take to complete
   * @param fileReaderBufferSize - If static files are served, how large
   *                               the read buffer should be. Not usually
   *                               modified
   * @throws std::invalid_argument if any of the settings are not valid
   */
  Config(
      std::vector<proxygen::HTTPServer::IPConfig> httpAddresses,
      size_t workerThreads,
      folly::Optional<std::string> publicDir = folly::Optional<std::string>(),
      std::chrono::milliseconds requestTimeout =
          std::chrono::milliseconds(kDefaultRequestTimeoutMs),
      size_t fileReaderBufferSize = kDefaultFileReaderBufferSize);

  /**
   * Returns the number of threads used for running event handlers
   */
  inline size_t getWorkerThreads() const noexcept { return workerThreads_; }

  /**
   * Returns the list of addresses / protocols used for listening for new
   * connections
   */
  inline std::vector<proxygen::HTTPServer::IPConfig> getHTTPAddresses() const
      noexcept {
    return httpAddresses_;
  };

  inline std::chrono::milliseconds getRequestTimeout() const noexcept {
    return requestTimeout_;
  }

  inline size_t getFileReaderBufferSize() { return fileReaderBufferSize_; }

  /**
   * Returns the path to the public directory if a public directory handler
   * should be created (else empty)
   */
  inline folly::Optional<boost::filesystem::path> getPublicDirectory() {
    if (addPublicDirectoryHandler_) {
      return publicDir_;
    } else {
      return folly::Optional<boost::filesystem::path>();
    }
  }
};
}
