#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include <folly/Format.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>

namespace nozomi {

class Config {
 private:
  std::vector<proxygen::HTTPServer::IPConfig> httpAddresses_;
  size_t workerThreads_;
  std::chrono::milliseconds requestTimeout_;

  void setHTTPAddresses(
      std::vector<proxygen::HTTPServer::IPConfig> httpAddresses);
  void setWorkerThreads(size_t workerThreads);
  void setRequestTimeout(std::chrono::milliseconds timeout);

 public:
  using Protocol = proxygen::HTTPServer::Protocol;

  /**
   * Creates a Config object for Servers
   * @param httpAddresses - A list of host / port / protocols to listen on
   * @param workerThreads - The number of threads to use for running event
   * handlers
   * @param requestTimeout - How long a single non-streaming request can
   *                         take to complete
   * @throws std::invalid_argument if any of the settings are not valid
   */
  Config(std::vector<std::tuple<std::string, uint16_t, Protocol>> httpAddresses,
         size_t workerThreads,
         std::chrono::milliseconds requestTimeout =
             std::chrono::milliseconds(30000));
  /**
   * Creates a Config object for Servers
   * @param httpAddresses - A list of host / port / protocols to listen on
   * @param workerThreads - The number of threads to use for running event
   * handlers
   * @param requestTimeout - How long a single non-streaming request can
   *                         take to complete
   * @throws std::invalid_argument if any of the settings are not valid
   */
  Config(std::vector<proxygen::HTTPServer::IPConfig> httpAddresses,
         size_t workerThreads,
         std::chrono::milliseconds requestTimeout =
             std::chrono::milliseconds(30000));

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
};
}
