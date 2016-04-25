#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <folly/Format.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpserver/HTTPServer.h>

namespace sakura {

class Config {
 private:
  std::vector<proxygen::HTTPServer::IPConfig> configs_;
  size_t workerThreads_;

  void setConfigs(std::vector<proxygen::HTTPServer::IPConfig> configs) {
    if (configs.size() == 0) {
      throw std::invalid_argument("At least one address must be provided");
    }
    configs_ = std::move(configs);
  }

  void setWorkerThreads(size_t workerThreads) {
    if (workerThreads == 0) {
      throw std::invalid_argument(folly::sformat(
          "Number of threads ({}) must be greater than zero", workerThreads));
    }
    workerThreads_ = workerThreads;
  }

 public:
  using Protocol = proxygen::HTTPServer::Protocol;

  Config(std::vector<std::tuple<std::string, uint16_t, Protocol>> configs,
         size_t workerThreads) {
    std::string host;
    uint16_t port;
    Protocol protocol;

    std::vector<proxygen::HTTPServer::IPConfig> newConfigs;
    newConfigs.reserve(configs.size());
    for (const auto& tup : configs) {
      std::tie(host, port, protocol) = tup;

      try {
        newConfigs.emplace_back(folly::SocketAddress{host, port}, protocol);
      } catch (const std::system_error& e) {
        throw std::invalid_argument(e.what());
      }
    }

    setConfigs(std::move(newConfigs));
    setWorkerThreads(workerThreads);
  }
  Config(std::vector<proxygen::HTTPServer::IPConfig> configs,
         size_t workerThreads) {
    setConfigs(std::move(configs));
    setWorkerThreads(workerThreads);
  }

  inline size_t getWorkerThreads() const { return workerThreads_; }
  inline std::vector<proxygen::HTTPServer::IPConfig> getHttpAddresses() const {
    return configs_;
  };
};
}
